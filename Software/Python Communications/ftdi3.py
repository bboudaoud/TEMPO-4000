#!/usr/bin/env python
'''This module is the first implementation of FTD2xx driver for the
FTDI USB chips. Initial implementation is for functions from the
dll required for present project'''
# (from: http://fluidmotion.dyndns.org/zenphoto/index.php?p=news&title=Python-interface-to-FTDI-driver-chip)

import sys
import ctypes as c


__FT_VERSION__ = '1.1'
__FT_LICENCE__ = 'LGPL3'
__FT_AUTHOR__ = 'Jonathan Roadley-Battin'


MAX_DESCRIPTION_SIZE = 256

FT_OK = 0
FT_LIST_NUMBER_ONLY = 0x80000000
FT_LIST_BY_INDEX = 0x40000000
FT_LIST_ALL = 0x20000000
FT_OPEN_BY_SERIAL_NUMBER = 1
FT_PURGE_RX = 1
FT_PURGE_TX = 2

FT_BITS_8 = 8
FT_BITS_7 = 7
FT_STOP_BITS_1 = 0
FT_STOP_BITS_2 = 2
FT_PARITY_NONE = 0
FT_PARITY_ODD = 1
FT_PARITY_EVEN = 2
FT_PARITY_MARK = 3
FT_PARITY_SPACE = 4

FT_FLOW_NONE = 0x0000
FT_FLOW_RTS_CTS = 0x0100
FT_FLOW_DTR_DSR = 0x0200
FT_FLOW_XON_XOFF = 0x0400

FT_FLAGS_OPENED = 0x00000001
FT_FLAGS_HISPEED = 0x00000002

class FtdiBitModes: # added by CJBH
    RESET         = 0x0
    ASYNC_BITBANG = 0x1
    MPSSE         = 0x2
    SYNC_BITBANG  = 0x4
    MCU_HOST      = 0x8
    FAST_SERIAL   = 0x10

ft_messages = ['OK',
                'INVALID_HANDLE',
                'DEVICE_NOT_FOUND',
                'DEVICE_NOT_OPENED',
                'IO_ERROR',
                'INSUFFICIENT_RESOURCES',
                'INVALID_PARAMETER',
                'INVALID_BAUD_RATE',
                'DEVICE_NOT_OPENED_FOR_ERASE',
                'DEVICE_NOT_OPENED_FOR_WRITE',
                'FAILED_TO_WRITE_DEVICE0',
                'EEPROM_READ_FAILED',
                'EEPROM_WRITE_FAILED',
                'EEPROM_ERASE_FAILED',
                'EEPROM_NOT_PRESENT',
                'EEPROM_NOT_PROGRAMMED',
                'INVALID_ARGS',
                'NOT_SUPPORTED',
                'OTHER_ERROR']


if sys.platform == 'win32':
    ft = c.windll.ftd2xx
else:
    ft = c.CDLL('libftd2xx.so')


######################################
##      FTDI exception classes      ##
######################################
class FTDeviceError(Exception):
    '''Exception class for FTDI function returns'''
    def __init__(self,msgnum):
        self.parameter = ft_messages[msgnum]
        self.status = msgnum
    def __str__(self):
        return repr(self.parameter)

#####################################
# CTYPES structure for DeviceInfo   #
#####################################
class DeviceListInfoNode(c.Structure):
    _fields_ = [    ('Flags',c.c_ulong),
                    ('Type',c.c_ulong),
                    ('ID',c.c_ulong),
                    ('LocID',c.c_ulong),
                    ('SerialNumber',(c.c_char * 16)),
                    ('Description',(c.c_char * 64)),
                    ('none',c.c_void_p),
                ]




####################################################
# Shared Lib functions via python function decorator
# for i in $(strings /usr/lib/libftd2xx.so.0.4.16 | grep FT_);do echo -e "@ftExceptionDecorator\ndef _${i/FT/PY}(*args):\n    return ft.$i(*args)\n";done
# Allows common exception routine to be performed on each fn
# Via Bash-liner additional fn can easily be added and specific pythonic fn added when needed
####################################################
def ftExceptionDecorator(f):
    def fn_wrap(*args):
        status = f(*args)
        if status == None:
            status = 18
        if status != FT_OK:
            raise FTDeviceError(status)
    return fn_wrap


# D2XX Classic Functions

if sys.platform != 'win32':
    @ftExceptionDecorator
    def _PY_SetVIDPID(*args):
        return ft.FT_SetVIDPID(*args)

    @ftExceptionDecorator
    def _PY_GetVIDPID(*args):
        return ft.FT_GetVIDPID(*args)

@ftExceptionDecorator
def _PY_CreateDeviceInfoList(*args):
    return ft.FT_CreateDeviceInfoList(*args)

@ftExceptionDecorator
def _PY_GetDeviceInfoList(*args):
    return ft.FT_GetDeviceInfoList(*args)

@ftExceptionDecorator
def _PY_GetDeviceInfoDetail(*args):
    return ft.FT_GetDeviceInfoDetail(*args)

@ftExceptionDecorator
def _PY_ListDevices(*args):
    return ft.FT_ListDevices(*args)

@ftExceptionDecorator
def _PY_Open(*args):
    return ft.FT_Open(*args)

@ftExceptionDecorator
def _PY_OpenEx(*args):
    return ft.FT_OpenEx(*args)

@ftExceptionDecorator
def _PY_Close(*args):
    return ft.FT_Close(*args)

@ftExceptionDecorator
def _PY_Read(*args):
    return ft.FT_Read(*args)

@ftExceptionDecorator
def _PY_Write(*args):
    return ft.FT_Write(*args)

@ftExceptionDecorator
def _PY_SetBaudRate(*args):
    return ft.FT_SetBaudRate(*args)

@ftExceptionDecorator
def _PY_SetDivisor(*args):
    return ft.FT_SetDivisor(*args)

@ftExceptionDecorator
def _PY_SetDataCharacteristics(*args):
    return ft.FT_SetDataCharacteristics(*args)

@ftExceptionDecorator
def _PY_SetTimeouts(*args):
    return ft.FT_SetTimeouts(*args)

@ftExceptionDecorator
def _PY_SetFlowControl(*args):
    return ft.FT_SetFlowControl(*args)

@ftExceptionDecorator
def _PY_SetDtr(*args):
    return ft.FT_SetDtr(*args)

@ftExceptionDecorator
def _PY_ClrDtr(*args):
    return ft.FT_ClrDtr(*args)

@ftExceptionDecorator
def _PY_SetRts(*args):
    return ft.FT_SetRts(*args)

@ftExceptionDecorator
def _PY_ClrRts(*args):
    return ft.FT_ClrRts(*args)

@ftExceptionDecorator
def _PY_GetModemStatus(*args):
    return ft.FT_GetModemStatus(*args)

@ftExceptionDecorator
def _PY_GetQueueStatus(*args):
    return ft.FT_GetQueueStatus(*args)

@ftExceptionDecorator
def _PY_GetDeviceInfo(*args):
    return ft.FT_GetDeviceInfo(*args)

@ftExceptionDecorator
def _PY_GetDriverVersion(*args):
    return ft.FT_GetDriverVersion(*args)

@ftExceptionDecorator
def _PY_GetLibraryVersion(*args):
    return ft.FT_GetLibraryVersion(*args)

@ftExceptionDecorator
def _PY_GetComPortNumber(*args):
    return ft.FT_GetComPortNumber(*args)

@ftExceptionDecorator
def _PY_GetStatus(*args):
    return ft.FT_GetStatus(*args)

# As far as I know, would not be possible to use system events
# @ftExceptionDecorator
# def _PY_SetEventNotification(*args):
#     return ft.FT_SetEventNotification(*args)

@ftExceptionDecorator
def _PY_SetChars(*args):
    return ft.FT_SetChars(*args)

@ftExceptionDecorator
def _PY_SetBreakOn(*args):
    return ft.FT_SetBreakOn(*args)

@ftExceptionDecorator
def _PY_SetBreakOff(*args):
    return ft.FT_SetBreakOff(*args)

@ftExceptionDecorator
def _PY_Purge(*args):
    return ft.FT_Purge(*args)

@ftExceptionDecorator
def _PY_ResetDevice(*args):
    return ft.FT_ResetDevice(*args)

@ftExceptionDecorator
def _PY_ResetPort(*args):
    return ft.FT_ResetPort(*args)

@ftExceptionDecorator
def _PY_CyclePort(*args):
    return ft.FT_CyclePort(*args)

@ftExceptionDecorator
def _PY_Rescan(*args):
    return ft.FT_Rescan(*args)

@ftExceptionDecorator
def _PY_Reload(*args):
    return ft.FT_Reload(*args)

@ftExceptionDecorator
def _PY_SetResetPipeRetryCount(*args):
    return ft.FT_SetResetPipeRetryCount(*args)

@ftExceptionDecorator
def _PY_StopInTask(*args):
    return ft.FT_StopInTask(*args)

@ftExceptionDecorator
def _PY_RestartInTask(*args):
    return ft.FT_RestartInTask(*args)

@ftExceptionDecorator
def _PY_SetDeadmanTimeout(*args):
    return ft.FT_SetDeadmanTimeout(*args)

# Undocumented functions listed in the API# @ftExceptionDecorator
# def _PY_IoCtl(*args):
#     return ft.FT_IoCtl(*args)
# 
# @ftExceptionDecorator
# def _PY_SetWaitMask(*args):
#     return ft.FT_SetWaitMask(*args)
# 
# @ftExceptionDecorator
# def _PY_WaitOnMask(*args):
#     return ft.FT_WaitOnMask(*args)


# EEPROM Programming Interface Functions

@ftExceptionDecorator
def _PY_ReadEE(*args):
    return ft.FT_ReadEE(*args)

@ftExceptionDecorator
def _PY_WriteEE(*args):
    return ft.FT_WriteEE(*args)

@ftExceptionDecorator
def _PY_EraseEE(*args):
    return ft.FT_EraseEE(*args)

@ftExceptionDecorator
def _PY_EE_Read(*args):
    return ft.FT_EE_Read(*args)

@ftExceptionDecorator
def _PY_EE_ReadEx(*args):
    return ft.FT_EE_ReadEx(*args)

@ftExceptionDecorator
def _PY_EE_Program(*args):
    return ft.FT_EE_Program(*args)

@ftExceptionDecorator
def _PY_EE_ProgramEx(*args):
    return ft.FT_EE_ProgramEx(*args)

@ftExceptionDecorator
def _PY_EE_UASize(*args):
    return ft.FT_EE_UASize(*args)

@ftExceptionDecorator
def _PY_EE_UARead(*args):
    return ft.FT_EE_UARead(*args)

@ftExceptionDecorator
def _PY_EE_UAWrite(*args):
    return ft.FT_EE_UAWrite(*args)


# Extended API Functions

@ftExceptionDecorator
def _PY_SetBitMode(*args): # added by CJBH
    return ft.FT_SetBitMode(*args)

@ftExceptionDecorator
def _PY_SetLatencyTimer(*args):
    return ft.FT_SetLatencyTimer(*args)

@ftExceptionDecorator
def _PY_SetUSBParameters(*args):
    return ft.FT_SetUSBParameters(*args)


################################################
# Start of pythonic functions for specific     #
# functionality around FTDI API                #
################################################
if sys.platform != 'win32':
    def set_VID_PID(dwVID, dwPID):
        _PY_SetVIDPID(c.c_ulong(dwVID), c.c_ulong(dwPID))
    
    def get_VID_PID():
        pdwVID = c.c_ulong();
        pdwPID = c.c_ulong();
        _PY_GetVIDPID(c.byref(pdwVID), c.byref(pdwPID))
        return {'VID': pdwVID.value, 'PID': pdwPID.value}

def list_devices():
    '''method to list devices connected.
    total connected and specific serial for a device position'''
    n = c.c_ulong()
    _PY_ListDevices(c.byref(n), None, c.c_ulong(FT_LIST_NUMBER_ONLY))

    if n.value:
        p_array = (c.c_char_p*(n.value + 1))()
        for i in range(n.value):
            p_array[i] = c.cast(c.c_buffer(64), c.c_char_p)
        _PY_ListDevices(p_array, c.byref(n), c.c_ulong(FT_LIST_ALL|FT_OPEN_BY_SERIAL_NUMBER ))
        return [ser for ser in p_array[:n.value]]
    else:
        return []
#------------------------------------------------------------------------------
def create_device_info_list():
    """Create the internal device info list and return number of entries"""
    lpdwNumDevs = c.c_ulong()
    _PY_CreateDeviceInfoList(c.byref(lpdwNumDevs))
    return lpdwNumDevs.value
#------------------------------------------------------------------------------
def get_device_info_detail(dev=0):
    """Get an entry from the internal device info list. """
    dwIndex = c.c_ulong(dev)
    lpdwFlags = c.c_ulong()
    lpdwType = c.c_ulong()
    lpdwID = c.c_ulong()
    lpdwLocId = c.c_ulong()
    pcSerialNumber = c.c_buffer(MAX_DESCRIPTION_SIZE)
    pcDescription = c.c_buffer(MAX_DESCRIPTION_SIZE)
    ftHandle = c.c_ulong()
    _PY_GetDeviceInfoDetail(dwIndex,
                                c.byref(lpdwFlags),
                                c.byref(lpdwType),
                                c.byref(lpdwID),
                                c.byref(lpdwLocId),
                                pcSerialNumber,
                                pcDescription,
                                c.byref(ftHandle))
    return {'Dev': dwIndex.value,
            'Flags': lpdwFlags.value,
            'Type': lpdwType.value,
            'ID': lpdwID.value,
            'LocId': lpdwLocId.value,
            'SerialNumber': pcSerialNumber.value,
            'Description': pcDescription.value,
            'ftHandle': ftHandle}
#------------------------------------------------------------------------------
def get_device_info_list():
    num_dev =  create_device_info_list()
    dev_info = DeviceListInfoNode * (num_dev + 1)
    pDest = c.pointer(dev_info())
    lpdwNumDevs = c.c_ulong()
    _PY_GetDeviceInfoList( pDest, c.byref(lpdwNumDevs))

    return_list = []
    data = pDest.contents
    for i in data:
        return_list.append({'Flags':i.Flags,'Type':i.Type,'LocID':i.LocID,'SerialNumber':i.SerialNumber,'Description':i.Description})
    return return_list[:-1]
#------------------------------------------------------------------------------
def open_ex(serial=''):
    '''open's FTDI-device by EEPROM-serial (prefered method).
    Serial fetched by the ListDevices fn'''
    ftHandle = c.c_ulong()
    dw_flags = c.c_ulong(FT_OPEN_BY_SERIAL_NUMBER)
    _PY_OpenEx(serial, dw_flags, c.byref(ftHandle))
    return FTD2XX(ftHandle)
#------------------------------------------------------------------------------



######################################
##     FTDI ctypes DLL wrapper      ##
######################################
class FTD2XX(object):
    '''class that implements a ctype interface to the FTDI d2xx driver'''
    def __init__(self, ftHandle):
        '''setup initial ctypes link and some varabled'''
        self.ftHandle = ftHandle
#------------------------------------------------------------------------------
    def set_baud_rate(self, dwBaudRate=921600):
        '''Set baud rate of driver, non-intelgent checking of allowed BAUD'''
        _PY_SetBaudRate(self.ftHandle, c.c_ulong(dwBaudRate))
        return None
#------------------------------------------------------------------------------
    def set_data_characteristics(self, uWordLength, uStopBits, uParity):
        '''Set bits per word, number of stop bits, and parity'''
        _PY_SetDataCharacteristics(self.ftHandle, c.c_ubyte(uWordLength),
                                   c.c_ubyte(uStopBits), c.c_ubyte(uParity))
        return None
#------------------------------------------------------------------------------
    def set_flow_control(self, usFlowControl, uXon, uXoff):
        '''Set flow control (none, RTS/CTS, etc.) and Xon/Xoff signals'''
        _PY_SetFlowControl(self.ftHandle, c.c_ushort(usFlowControl),
                           c.c_ubyte(uXon), c.c_ubyte(uXoff))
        return None
#------------------------------------------------------------------------------
    def set_timeouts(self, dwReadTimeout=100, dwWriteTimeout=100):
        '''setup timeout times for TX and RX'''
        _PY_SetTimeouts(self.ftHandle, c.c_ulong(dwReadTimeout), c.c_ulong(dwWriteTimeout))
        return None
#------------------------------------------------------------------------------
    def set_latency_timer(self, ucTimer=16): # added by CJBH
        '''setup latency timer'''
        _PY_SetLatencyTimer(self.ftHandle, c.c_ubyte(ucTimer))
        return None
#------------------------------------------------------------------------------
    def set_bit_mode(self, ucMask=0, ucMode=0): # added by CJBH
        '''setup bit mode'''
        _PY_SetBitMode(self.ftHandle, c.c_ubyte(ucMask), c.c_ubyte(ucMode))
        return None
#------------------------------------------------------------------------------
    def set_usb_parameters(self, dwInTransferSize=4096, dwOutTransferSize=0):
        '''set the drivers input and output buffer size'''
        _PY_SetUSBParameters(self.ftHandle, c.c_ulong(dwInTransferSize), c.c_ulong(dwOutTransferSize))
        return None
#------------------------------------------------------------------------------
    def purge(self, to_purge= 'TXRX'):
        '''purge the in and out buffer of driver.
            Valid arguement = TX,RX,TXRX'''
        if to_purge == 'TXRX':
            dwMask = c.c_ulong(FT_PURGE_RX|FT_PURGE_TX)
        elif to_purge == 'TX':
            dwMask = c.c_ulong(FT_PURGE_TX)
        elif to_purge == 'RX':
            dwMask = c.c_ulong(FT_PURGE_RX)

        _PY_Purge(self.ftHandle, dwMask)
        return None
#------------------------------------------------------------------------------
    def get_queue_status(self):
        '''returns the number of bytes in the RX buffer
        else raises an exception'''
        lpdwAmountInRxQueue = c.c_ulong()
        _PY_GetQueueStatus(self.ftHandle, c.byref(lpdwAmountInRxQueue))
        return lpdwAmountInRxQueue.value
#------------------------------------------------------------------------------
    def write(self, lpBuffer=''):
        '''writes the string-type "data" to the opened port.'''
        lpdwBytesWritten = c.c_ulong()
        _PY_Write(self.ftHandle, lpBuffer, len(lpBuffer), c.byref(lpdwBytesWritten))
        return lpdwBytesWritten.value
#------------------------------------------------------------------------------
    def read(self, dwBytesToRead, raw=True):
        '''Read in int-type of bytes. Returns either the data
        or raises an exception'''
        lpdwBytesReturned = c.c_ulong()
        lpBuffer = c.c_buffer(dwBytesToRead)
        _PY_Read(self.ftHandle, lpBuffer, dwBytesToRead, c.byref(lpdwBytesReturned))
        return lpBuffer.raw[:lpdwBytesReturned.value] if raw else lpBuffer.value[:lpdwBytesReturned.value]
#------------------------------------------------------------------------------
    def read_available(self,max_bytes=None):
        bytes_to_read = self.get_queue_status()
        if max_bytes is not None:
            bytes_to_read = min(bytes_to_read, max_bytes)
        return self.read(bytes_to_read)
#------------------------------------------------------------------------------
    def reset_device(self):
        '''closes the port.'''
        _PY_ResetDevice(self.ftHandle)
        return None
#------------------------------------------------------------------------------
    def close(self):
        '''closes the port.'''
        _PY_Close(self.ftHandle)
        return None
