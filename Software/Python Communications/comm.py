"""Low level interface to TEMPO Flash nodes

Provides a TempoCommunicator class that understands the request and
response framing structure used by TEMPO Flash. Each command supported
by the TEMPO node is presented as an instance method of the class.

Numerous exception classes are included to map various communication
errors, such as timeouts and bad response codes.

Finally, the open_unopened_nodes() method provides a simple search
routine to locate any connected TEMPO nodes via currently-unopened
FTDI USB devices connected to the system.
"""
import struct
import datetime
import time
import ftdi3
from fletcher import fletcher16
import threading
import asyncutil

# Constants defining events used in NodeDiscoveryDaemon
NODE_FOUND = "<NodeCommFound>"
NODE_LOST = "<NodeCommLost>"

_BAUD_RATE = 115200

# Packetization constants
_START_OF_REQUEST = b'S'
_REQUEST_ARG_LEN = 16
_REQUEST_ARG_PAD = struct.pack('<B', 0x00)
_START_OF_REPLY = b'R'
_REPLY_HEADER_LEN = struct.calcsize('<BHHHH')

# Reply header response codes
_SUCCESS = 0
_CORRUPT_REQUEST = 1
_NEED_HANDSHAKE = 2
_NO_SUCH_COMMAND = 3
_FAILURE_READ_ONLY = 4
_FAILURE_GENERAL = 5
_BAD_ARGUMENT = 6

# Command words
_CMD_HANDSHAKE = b'H'
_CMD_STATUS = b'?'
_CMD_GETCLK = b'T'
_CMD_SETCLK = b'C'
_CMD_NODE_ID = b'N'
_CMD_VERSION = b'V'
_CMD_CARD_ID = b'*'
_CMD_EN_DATA_COLL = b'$'
_CMD_DIS_DATA_COLL = b'W'
_CMD_SET_SR = b'R'
_CMD_SECTOR = b'S'
_CMD_SECTOR_RANGE = b'B'
_CMD_GET_ACCEL = b'A'
_CMD_GET_GYRO = b'G'
_CMD_LED_ON = b'1'
_CMD_LED_OFF = b'0'
_CMD_OVWRT = b'O'
_CMD_INIT_CARD = b'I'

# Constants relating to sectors
TRUE_SECTOR_SIZE = 512
MAX_SECTORS_PER_REQUEST = 127
NUM_SECTORS = 2*10**9 // TRUE_SECTOR_SIZE   # 2 bil. byte card

# Exception classes
class CommError(Exception):
    """Various errors can occur during communication with TEMPO"""
    pass

class CommResponseError(CommError):
    """Indicates a non-success response code in the reply header"""
    pass

class CorruptSector(Exception):
    """Sector data did not match its checksum"""
    pass

def get_node():
    #note this is a temp function
    ns = discover_nodes()
    return ns[0][1]

def discover_nodes():
    """Opens and returns and not-yet-opened TEMPO nodes
    
    Discovers all unopened FTDI devices connected, and attempts to
    contact a TEMPO node at each one by performing a handshake. If the
    handshake succeeds, the TempoCommunicator is added to the list
    of returned TEMPO nodes.
    """ 
    nodes_found = []
    
    try:
        dev_infos = ftdi3.get_device_info_list()
    except ftdi3.FTDeviceError:
        return nodes_found
    
    for info in dev_infos:
        if not (info['Flags'] & ftdi3.FT_FLAGS_OPENED):
            try:
                device = ftdi3.open_ex(info['SerialNumber'])
            except ftdi3.FTDeviceError:
                # device var not assigned, continue to next iteration
                continue
            # Do this try separately, now that device is assigned
            try:
                device.reset_device()
                device.set_baud_rate(_BAUD_RATE)
                device.set_data_characteristics(ftdi3.FT_BITS_8, 
                                                ftdi3.FT_STOP_BITS_1, 
                                                ftdi3.FT_PARITY_NONE)
                device.set_flow_control(ftdi3.FT_FLOW_NONE, 0, 0)
                device.purge(to_purge='TXRX')
                tempo_comm = TempoCommunicator(device)
                tempo_comm.handshake()
                node_id = tempo_comm.get_node_id()
            except (ftdi3.FTDeviceError, CommError):
                device.close()
            except:
                device.close()
                raise
            else:
                nodes_found.append((node_id, tempo_comm))
    
    return nodes_found


class NodeDiscoveryDaemon(asyncutil.Observable, threading.Thread):
    """
    Service that continually looks for new nodes and reports them via
    an event, while occasionally also looking for lost nodes and closing
    and reporting them via another event.
    """
    def __init__(self, check_interval=500):
        # Initialize mixin superclasses
        events = (NODE_FOUND, NODE_LOST)
        asyncutil.Observable.__init__(self, events)
        
        self._check_interval = check_interval / 1000
        self._observers = {NODE_FOUND: [], NODE_LOST: []}
        self._observer_lock = threading.Lock()
        self._kill_evt = threading.Event()
        self._comms = {}
        threading.Thread.__init__(self)
        
        # Set to daemon so will automatically terminate when calling
        # thread terminates. Must be done after superclass __init__
        self.daemon = True
    
    def signal_kill(self):
        self._kill_evt.set()
    
    def run(self):
        while not self._kill_evt.is_set():
            # Discover new nodes
            new_nodes = discover_nodes()
            for node_id, comm in new_nodes:
                # Close and remove any old handle to this node
                if node_id in self._comms:
                    self._comms.pop(node_id).close()
                    self._notify_observers(NODE_LOST, node_id)
                
                # Save the new one and notify everybody it exists
                self._comms[node_id] = comm
                self._notify_observers(NODE_FOUND, node_id, comm)
            
            # Every 10 seconds, ping all the nodes
            # Must actively ping and close nodes so that we recognize
            #    when a node has been disconnected and reconnected,
            #    or another node connected in its place on the dock.
            if datetime.datetime.utcnow().second % 10 == 0:
                node_ids_to_drop = []
                for node_id, comm in self._comms.items():
                    try:
                        comm.get_node_id()
                    except (ftdi3.FTDeviceError, CommError):
                        self._comms[node_id].close()
                        node_ids_to_drop.append(node_id)
                        self._notify_observers(NODE_LOST, node_id)
                
                # Have to wait and delete after iteration over dict
                for node_id in node_ids_to_drop:
                    del self._comms[node_id]
            
            # Yield for a bit
            time.sleep(self._check_interval)


class TempoCommunicator:
    """Sends commands to TEMPO and parses responses.
    
    Each public function corresponds to a TEMPO command and knows how
    to parse the specific reply data from that command.
    
    This class requires an open FTDI device handle (FTD2XX object) in
    order to communicate with a TEMPO node.
    """
    def __init__(self, device=None):
        """Initialize with an opened, configured FTDI device"""
        #TODO: Consider moving some of the FTDI config code here
        self.device = device
        self._closed = False
        self._msg_lock = threading.Lock()
    
    @property
    def closed(self):
        return self._closed
    
    def close(self):
        if not self.closed:
            self.device.close()
            self._closed = True
    
    def _tempo_message(self, command, argument=b'',
                        timeout=1000, check_interval=1):
        with self._msg_lock:
            return self._tempo_message_unsafe(command, argument,
                                              timeout, check_interval)
    
    def _tempo_message_unsafe(self, command, argument=b'',
                              timeout=1000, check_interval=1):
        """
        Send a message to a TEMPO 3.2F node and get the reply.
        
        Packetizes and sends a command message, waits timeout for the
        reply, verifies the checksum and response code of the reply
        packet, and returns the reply message extracted from the reply
        packet as a raw byte-array object, intended for use with
        struct.unpack().
        
        Arguments:
        command -- a TEMPO 3.2F command as integer, in the uint range
                   0..65535
        argument -- a byte-array argument, best produced by
                    struct.pack()
        timeout -- time in ms to wait for a reply (default 1000)
        check_interval -- time in ms to wait between checks for reply
        
        Exceptions:
        Raises FTDeviceError for FTDI library errors, otherwise a bad
        checksum or bad response code from the TEMPO node will raise a
        CommError or subclass Error with embedded message.
        
        """
        # Package up the command/request and send it
        cmd = struct.pack('<c', command)
        ck_cmd = struct.pack('<cx', command)
        arg_padding = _REQUEST_ARG_PAD * (_REQUEST_ARG_LEN - len(argument))
        ck_packet = ck_cmd + argument + arg_padding
        packet = cmd + argument + arg_padding
        cksum = fletcher16(ck_packet).digest()
        packet += struct.pack('<H',cksum)
        packet = _START_OF_REQUEST + packet
        self.device.purge()
#        self.device.write(packet)
        # For now, need to insert a 1-ms delay between characters
        for b in packet:
            ##time.sleep(0.001)
            self.device.write(bytes([b]))
        
        # Set the timeout deadline for getting a full reply
        timeout_delta = datetime.timedelta(milliseconds=timeout)
        deadline = datetime.datetime.utcnow() + timeout_delta
        
        # Scan for a potential reply packet header (start char & proper length)
        got_reply_header = False
        read_data = b''
        while not got_reply_header:
            bytes_needed = _REPLY_HEADER_LEN - len(read_data)
            # TODO: Raise CommError if FTDI caught 
            read_data += self.device.read_available(max_bytes=bytes_needed)
            read_data = read_data[read_data.find(_START_OF_REPLY):]
            if len(read_data) >= _REPLY_HEADER_LEN:
                got_reply_header = True
            elif datetime.datetime.utcnow() >= deadline:
                raise CommError("timed out waiting for reply header")
            time.sleep(check_interval / 1000)
        
        reply_header = read_data[len(_START_OF_REPLY):]
        (echo_command, response_code, payload_len, checksum) = \
            struct.unpack('<cxHHH', reply_header)
        
        # Validate the reply packet header and check the response code
        if checksum != fletcher16(reply_header[:-2]).digest():
            raise CommError("receiver corrupt reply header")
        elif echo_command != command:
            raise CommError("reply header echo did not match command")
        elif response_code != _SUCCESS:
            if response_code == _CORRUPT_REQUEST:
                raise CommResponseError("request sent was corrupted")
            elif response_code == _NEED_HANDSHAKE:
                raise CommResponseError("handshake required first")
            elif response_code == _NO_SUCH_COMMAND:
                raise CommResponseError("invalid command sent")
            elif response_code == _FAILURE_READ_ONLY:
                raise CommResponseError("node in read-only mode")
            elif response_code == _FAILURE_GENERAL:
                raise CommResponseError("general failure")
            elif response_code == _BAD_ARGUMENT:
                raise CommResponseError("request contained a bad argument")
            else:
                raise CommResponseError("unrecognized response code")
        
        if payload_len == 0:
            payload = None
        else:
            got_payload = False
            payload_data = b''
            while not got_payload:
                bytes_needed = payload_len - len(payload_data) 
                payload_data += self.device.read_available(
                                                       max_bytes=bytes_needed)
                if len(payload_data) >= payload_len:
                    got_payload = True
                elif datetime.datetime.utcnow() >= deadline:
                    raise CommError("timed out waiting for payload")
                time.sleep(check_interval / 1000)
            
            (checksum,) = struct.unpack('<H', payload_data[-2:])
            payload = payload_data[:-2]
            
            if checksum != fletcher16(payload).digest():
                raise CommError("received corrupt payload")
            
        return payload
    
    def handshake(self):
        """
        Perform handshake that must occur before node responds to other
        commands. The handshake is valid until the node loses contact
        with the charger.
        """
        self._tempo_message(_CMD_HANDSHAKE)
    
    def get_status(self):
        """
        Fetch a handful of basic node status flags.
        """
        reply_data = self._tempo_message(_CMD_STATUS)
        status_tuple = struct.unpack('<??b?', reply_data)
        return {'ReadOnly': status_tuple[0],
                'DataCollectionEnabled': status_tuple[1],
                'ClockCalibrationRequested': status_tuple[2],
                'CardFull': status_tuple[3]}
    
    def get_node_id(self):
        """
        Fetch the node's permanently assigned positive integer ID.
        """
        reply_data = self._tempo_message(_CMD_NODE_ID)
        (node_id,) = struct.unpack('<H', reply_data)
        return node_id
    
    def get_card_id(self):
        """
        Fetch the node's current 16-byte MMC card ID.
        """
        return self._tempo_message(_CMD_CARD_ID)
    
    def set_for_overwrite(self):
        """
        Sets the card to overwrite its current data & metadata by
        resetting the sector #s and indices in the FirstSectorInfo.
        Serial numbers are not reset, because they must continue
        counting after overwrite.
        """
        self._tempo_message(_CMD_OVWRT)
    
    def get_node_clock(self):
        """
        Returns the sysCounter value, intended for PC to call repeatedly
        to perform clock sync.
        """
        reply_data = self._tempo_message(_CMD_GETCLK)
        print(reply_data)
        (year, month, day, hour, min, sec) = struct.unpack('<HHHHHH', reply_data)
        print((year, month, day, hour, min, sec))
        t = datetime.datetime(year, month, day, hour, min, sec)
        return t
    
    def get_flash_sector(self, sector_number, verify_checksum=True,
                         strip_checksum=True, retry_limit=0):
        """
        Returns the requested 512-byte sector of data from the card,
        including the on-card checksum, and also echos the requested
        sector number.
        """
        cmd_arg = struct.pack('<I', sector_number)
        
        # Command retry loop, will ignore CommError retry_limit times 
        retry_limit += 1    # We get 1 + retry_limit tries
        got_successful_reply = False
        while not got_successful_reply:
            try:
                reply_data = self._tempo_message(_CMD_SECTOR, cmd_arg)
            except CommError:
                retry_limit -= 1
                if retry_limit < 1:
                    raise
            else:
                got_successful_reply = True
        
        echo_sector_num = reply_data[:4]
        sector = reply_data[len(echo_sector_num):]
        checksum = sector[-2:]
        sector_data = sector[:-len(checksum)]
        
        if (sector_number,) != struct.unpack('<I', echo_sector_num):
            raise CommError("sector number echoed did not match request")
        if verify_checksum:
            (checksum,) = struct.unpack('<H', checksum)
            if checksum != fletcher16(sector_data).digest():
                raise CorruptSector()
        if strip_checksum:
            return sector_data
        else:
            return sector
    # TODO: Decide whether or not to use this
    def get_flash_sector_range(self, start_sector, sector_count,
                               verify_checksum=True, strip_checksum=True,
                               retry_limit=0):
        """
        Returns the requested group of 512-byte sectors of data from the
        card, including the on-card checksum, and also echos the requested
        starting sector number.
        """
        cmd_arg = struct.pack('<IH', start_sector, sector_count)
        
        approx_payload_bits = sector_count * TRUE_SECTOR_SIZE * 8
        time_to_allow = 2000 + 1000 * (approx_payload_bits / _BAUD_RATE)
        
        # Command retry loop, will ignore CommError retry_limit times 
        retry_limit += 1    # We get 1 + retry_limit tries
        got_successful_reply = False
        while not got_successful_reply:
            try:
                reply_data = self._tempo_message(_CMD_SECTOR_RANGE,
                                                 cmd_arg,
                                                 timeout=time_to_allow)
            except CommError:
                retry_limit -= 1
                if retry_limit < 1:
                    raise
            else:
                got_successful_reply = True
        
        echo_sector_num = reply_data[:4]
        if (start_sector,) != struct.unpack('<I', echo_sector_num):
            raise CommError("sector number echoed did not match request")
        
        sector_list = []
        for i in range(sector_count):
            offset = len(echo_sector_num) + i * TRUE_SECTOR_SIZE
            sector = reply_data[offset:offset + TRUE_SECTOR_SIZE]
            checksum = sector[-2:]
            sector_data = sector[:-len(checksum)]
            
            if verify_checksum:
                (checksum,) = struct.unpack('<H', checksum)
                if checksum != fletcher16(sector_data).digest():
                    raise CorruptSector()
            
            if strip_checksum:
                sector_list.append(sector_data)
            else:
                sector_list.append(sector)
        
        return sector_list
    
    def get_firmware_version(self):
        """
        Fetches a structured TEMPO firmware version number.
        
        Version fields: (Major Rev, Minor Rev, Major Update,
                         Minor Update)
        """
        reply_data = self._tempo_message(_CMD_VERSION)
        fw_ver = struct.unpack('<BBBB', reply_data)
        return fw_ver
    
    
    #TODO: PORT THIS CODE!!!
    def init_flash_card(self, node_id, session_serial, timing_log_serial,
                        timing_epoch_number):
        """
        (Re-)initialize the card. Necessary when node built, card
        replaced, or to clear a read-only condition. PC must supply the
        serial numbers to continue counting from, which the user must
        carefully try to figure out after a read-only (card corrupt)
        situation, as we don't want to re-use log serial numbers. This
        command causes a specially defined sequence of padding to be
        written to the first sector, which the node typically searches
        for on boot to determine whether it has been properly
        initialized yet. This command automatically performs an
        overwrite and clears the read-only flag.
        
        Arguments:
        ft_device -- an open FTD2XX object to communicate through
        session_serial -- 
        timing_log_serial --
        time_epoch_number --
        """
        cmd_arg = struct.pack('<HHHHH', node_id, session_serial,
                              timing_log_serial, timing_epoch_number)
        self._tempo_message(_CMD_INIT_CARD, cmd_arg)
        
    def get_gyro_triplet(self):
        reply_data=self._tempo_message(_CMD_GET_GYRO)
        gyro_triplet=struct.unpack('<HHH', reply_data)
        return gyro_triplet
    
    def set_sampling_rate(self, sampling_rate):
        """
        Set the new sampling rate of the node, which will persist over
        device resets until changed via this command.
        
        The sampling rate argument should be a constant supplied by this
        module, such as SAMP_RATE_128, and any invalid value will result
        in a CommResponseError for passing a bad argument.
        """
        cmd_arg = struct.pack('<H', sampling_rate)
        self._tempo_message(_CMD_SET_SR, cmd_arg)
    
    def set_led_on(self):
        """"
        Turn on the on-board LED: DUMMY FUNCTION
        """
        self._tempo_message(_CMD_LED_ON)
    
    def set_led_off(self):
        self._tempo_message(_CMD_LED_OFF)
        
    def set_node_clock(self, pc_datetime = None):
        if pc_datetime is None:
            t = datetime.datetime.utcnow()
            t_tuple = (t.year, t.month, t.day, t.hour, t.minute, t.second)
            print(t_tuple)
            
        else:
            t_tuple = (pc_datetime.year, pc_datetime.month, pc_datetime.day,
                       pc_datetime.hour, pc_datetime.min, pc_datetime.sec)
        cmd_arg = struct.pack('<HHHHHH', *t_tuple)
        self._tempo_message(_CMD_SETCLK, cmd_arg)
    
    def enable_data_collection(self):
        """
        Enable data collection when not on charger. This will persist over
        device resets until changed via disable data collection command.
        """
        self._tempo_message(_CMD_EN_DATA_COLL)
        
    def disable_data_collection(self):
        """
        Disable data collection when not on charger. This will persist over
        device resets until changed via enable data collection command.
        """
        self._tempo_message(_CMD_DIS_DATA_COLL)
    
