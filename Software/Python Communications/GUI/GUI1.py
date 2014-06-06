'''
Created on Mar 3, 2014
This is an internal use GUI for sending commands to tempo4k nodes via USB.
A separate readme.txt should be included (TODO)
@author: Bill Devine
'''
import tkinter as tk
from tkinter import ttk
import comm
import FileSystem
root=tk.Tk()

class Application:
   
    def __init__(self, master):
        self.nodes=[]
        #Initiate container
        
        
        Frame1=tk.Frame(master)
        Frame1.pack()
        
        #Button to Find Nodes
        self.FindNodeButton=tk.Button(Frame1, text="Detect Nodes", fg="black", command=self.detectNodes)
        self.FindNodeButton.grid(row=0, column=1)
        
     
        
        # List of Nodes
        self.NodeList=tk.Listbox(Frame1)
        self.NodeList.grid(row=1, column=1)
        
        self.selectNodeButton=tk.Button(Frame1, text="Select Node", fg="black", command=self.selectNode)
        self.selectNodeButton.grid(row=0, column=2)
        #Enter Payload
        self.PayloadEntryBox=tk.Entry(Frame1, width=20)
        self.PayloadEntryBox.grid(row=1, column=2)
        
        #Drop Down Menu for choosing a command
        self.var=tk.StringVar()
        self.var.set("Choose A Command")      
        self.CommandList=tk.OptionMenu(Frame1, self.var, "Get Node Status", "Get Node Clock", "Set Node Clock",
                                       "Get Node ID", "Get Firmware Version", "Get Flash Card ID", "Enable Data Collection",
                                       "Disable Data Collection", "Set Node Sampling Rate", "Get Sector", "Get Range of Sectors",
                                       "Get Accelerometer Triplet", "Get Gyroscope Triplet", "Turn on LED", "Turn off LED")
        self.CommandList.grid(row=2, column=1)
        
        
        
        #Button to send a command
        self.SendCommandButton=tk.Button(Frame1, text="Send Command", fg="green", command=self.sendCommand)
        self.SendCommandButton.grid(row=3, column=1)
        
        #Button to Quit Program
        self.QuitButton=tk.Button(Frame1, text="Quit", fg="red", command=Frame1.quit)
        self.QuitButton.grid(row=3, column=2)
        
        self.ForensicDataButton=tk.Button(Frame1, text="Show Data from Sector", fg="green", command=self.openDataWindow)
        self.ForensicDataButton.grid(row=3, column=3)


       
    def openDataWindow(self):
        print('this opens a window to parse data sectors')
        if self.nodes!=[]:
            for node in self.nodes[:][0]:
                node[1].close()
        self.dataWindow=tk.Toplevel(root)
        self.app=DataWindow(self.dataWindow)   
         
    def sendCommand(self):
        print("Command Sending...")
        CommandString=self.var.get()
        print(self.nodes)
        Node=self.selectNode()
        print(Node)
        Payload=self.PayloadEntryBox.get()
        returnVal=self.parseCommandString(CommandString, Node, Payload)
        print(returnVal)
            
    def selectNode(self):
        i=0
        print(self.nodes)
        for node in self.nodes[:][0]:
            if self.NodeList.get("active")== ("Node "+str(node[0])):
                return node[1]
            else:
                i=i+1
                continue
        
            
            
    def parseCommandString(self, CommandString, Node, Payload):
        if CommandString=="Get Node Status":
            return comm.TempoCommunicator.get_status(Node)
        elif CommandString=="Get Node Clock":
            return comm.TempoCommunicator.get_node_clock(Node)
        elif CommandString=="Set Node Clock":
            return comm.TempoCommunicator.set_node_clock(Node)
        elif CommandString=="Get Node ID":
            return comm.TempoCommunicator.get_node_id(Node)
        elif CommandString=="Get Firmware Version":
            return comm.TempoCommunicator.get_firmware_version(Node)
        elif CommandString=="Get Flash Card ID":
            return Node.get_card_id()
        elif CommandString=="Enable Data Collection":
            return comm.TempoCommunicator.enable_data_collection(Node)
        elif CommandString== "Disable Data Collection":
            return comm.TempoCommunicator.disable_data_collection(Node)
        elif CommandString=="Set Node Sampling Rate":
            return comm.TempoCommunicator.set_sampling_rate(Node, Payload)
        elif CommandString=="Get Sector":
            return comm.TempoCommunicator.get_flash_sector(Node, Payload)
        elif CommandString=="Get Range of Sectors":
            return "TODO"
        elif CommandString=="Get Accelerometer Triplet":
            return "TODO"
        elif CommandString=="Get Gyroscope Triplet":
            return comm.TempoCommunicator.get_gyro_triplet(Node)
        elif CommandString=="Turn on LED":
            comm.TempoCommunicator.set_led_on(Node)
            return "TODO"
        elif CommandString=="Turn off LED":
            comm.TempoCommunicator.set_led_off(Node)
            return "TODO"
        else :
            return b'X'
        
    def detectNodes(self):
        print("Detection...BEEPBOOPBOP")
        if self.nodes!=[]:
            for node in self.nodes[:][0]:
                node[1].close()
               
        l=comm.discover_nodes()
        print(l)
        self.nodes.append(l)
        print(self.nodes)
        print(self.nodes[0])
        self.NodeList.delete(0, "end")
        for node in self.nodes[:][0]:
            print(node[0])
            print(node[1])
            self.NodeList.insert("end", "Node "+str(node[0]))
        
  
class DataWindow:
    
    def __init__(self, master):
        self.nodes=[]
        self.master=master
        self.DataFrame=ttk.Frame(self.master)
        self.DataFrame.pack()
        self.ActiveNode=[]
        self.ActiveFilesystem=FileSystem.Filesystem()
        
        self.SelectionFrame=ttk.LabelFrame(self.DataFrame, text="Session Selection",  padding="10 10 12 12")
        self.SelectionFrame.grid(column=1, row=1)
        
        self.ActiveSessionFrame=ttk.LabelFrame(self.SelectionFrame, text="Active Session",  padding="10 10 12 12")
        self.ActiveSessionFrame.grid(column=1, row=4)
        
        #self.setupSelectionFrame(self.SelectionFrame)
        self.ActiveNodesList=tk.Listbox(self.SelectionFrame)
        self.ActiveNodesList.grid(column=1, row=1)
        
        ttk.Label(self.SelectionFrame, text="Active Nodes").grid(column=1, row=0)
        
        self.DetectNodeButton=tk.Button(self.SelectionFrame, text="Detect Nodes", command=self.detectNodes)
        self.DetectNodeButton.grid(column=1, row=2)
        
        self.ChooseNodeButton=tk.Button(self.SelectionFrame, text="Choose Node", command=self.chooseNode)
        self.ChooseNodeButton.grid(column=2, row=1)
        
        self.ChooseSessionButton=tk.Button(self.SelectionFrame, text="Choose Session", command=self.chooseSession)
        self.ChooseSessionButton.grid(column=2, row=2)
        
        self.CardInfoFrame=ttk.LabelFrame(self.SelectionFrame, text="Active Node Info",  padding="10 10 12 12")
        self.CardInfoFrame.grid(column=2, row=3)
        
        self.SessionList=tk.Listbox(self.SelectionFrame)
        self.SessionList.grid(column=3, row=1)
        
        ttk.Label(self.SelectionFrame, text="Sessions on Card").grid(column=3, row=0)
        
        self.DetectSessionButton=tk.Button(self.SelectionFrame, text="Detect Sessions", command=self.detectSessions)
        self.DetectSessionButton.grid(column=3, row=2)
        
        #setup card info Frame
        ttk.Label(self.CardInfoFrame, text="Node ID Number: ").grid(column=1 , row=1)
        ttk.Label(self.CardInfoFrame, text="Card Status: ").grid(column=1 , row=2)
        ttk.Label(self.CardInfoFrame, text="Current Timing Epoch: ").grid(column=1 , row=3)
        ttk.Label(self.CardInfoFrame, text="Last Data Sector Written: ").grid(column=1 , row=4)
        ttk.Label(self.CardInfoFrame, text="Last Session Information Sector: ").grid(column=1 , row=5)
        ttk.Label(self.CardInfoFrame, text="Start Sector: ").grid(column=1 , row=6)
        ttk.Label(self.CardInfoFrame, text="Card Initialization Time: ").grid(column=1 , row=7)
        ttk.Label(self.CardInfoFrame, text="Card Notes: ").grid(column=1 , row=8)
        
        self.NodeIDNumber=tk.StringVar("")
        self.CardStatus=tk.StringVar("")
        self.Epoch=tk.StringVar("")
        self.LastData=tk.StringVar("")
        self.LastSession=tk.StringVar("")
        self.StartSector=tk.StringVar("")
        self.CardInitTime=tk.StringVar("")
        self.CardNotes=tk.StringVar("")
        
        ttk.Label(self.CardInfoFrame, textvariable=self.NodeIDNumber).grid(column=2 , row=1)
        ttk.Label(self.CardInfoFrame, textvariable=self.CardStatus).grid(column=2 , row=2)
        ttk.Label(self.CardInfoFrame, textvariable=self.Epoch).grid(column=2 , row=3)
        ttk.Label(self.CardInfoFrame, textvariable=self.LastData).grid(column=2 , row=4)
        ttk.Label(self.CardInfoFrame, textvariable=self.LastSession).grid(column=2 , row=5)
        ttk.Label(self.CardInfoFrame, textvariable=self.StartSector).grid(column=2 , row=6)
        ttk.Label(self.CardInfoFrame, textvariable=self.CardInitTime).grid(column=2 , row=7)
        ttk.Label(self.CardInfoFrame, textvariable=self.CardNotes).grid(column=2 , row=8)
      
        self.SelectionFrame.pack()
        
        self.updateActiveNodeInfo()
        
        #set up Active Session Frame
        
      

        
        self.DataFrame.pack()
        
        
    def detectSessions(self):
        print("this will detect sessions")
        
    def chooseNode(self):
        print("Chooses a node")
        i=0
        print(self.nodes)
        for node in self.nodes[:][0]:
            if self.NodeList.get("active")== ("Node "+str(node[0])):
                self.ActiveNode=(node[1])
                self.updateActiveNodeInfo()
                
            else:
                i=i+1
                continue
        
    def chooseSession(self):
        print("choose a session")
        self.sessionDisplay=tk.Toplevel(root)
        SessionWindow(self.sessionDisplay) 
        
    def updateActiveNodeInfo(self):
        CardInfoSector=self.getCardInfoSector()
        self.ActiveFilesystem.parseCardInfo(CardInfoSector)
        self.NodeIDNumber.set(getattr(self.ActiveFilesystem.ActiveCardInfoSector, 'NodeIDNumber'))
        self.CardStatus.set(getattr(self.ActiveFilesystem.ActiveCardInfoSector, 'CardStatus'))
        self.Epoch.set(getattr(self.ActiveFilesystem.ActiveCardInfoSector, 'TimingEpoch'))
        self.LastData.set(getattr(self.ActiveFilesystem.ActiveCardInfoSector, 'LastDataSectorWritten'))
        self.LastSession.set(getattr(self.ActiveFilesystem.ActiveCardInfoSector, 'LastSessionInfoSectorWritten'))
        self.StartSector.set(getattr(self.ActiveFilesystem.ActiveCardInfoSector, 'StartSector'))
        self.CardInitTime.set(getattr(self.ActiveFilesystem.ActiveCardInfoSector, 'CardInitializationTime'))
        self.CardNotes.set(getattr(self.ActiveFilesystem.ActiveCardInfoSector, 'Notes'))
        
    def getCardInfoSector(self):
        return b'ABCDEFGHIJKLMNOPQRSTUVWXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'

            
    def detectNodes(self):
        print("this will detect nodes")
        print("Detection...BEEPBOOPBOP")
        if self.nodes!=[]:
            for node in self.nodes[:][0]:
                node[1].close()
               
        l=comm.discover_nodes()
        print(l)
        self.nodes.append(l)
        print(self.nodes)
        print(self.nodes[0])
        self.ActiveNodesList.delete(0, "end")
        for node in self.nodes[:][0]:
            print(node[0])
            print(node[1])
            self.ActiveNodesList.insert("end", "Node "+str(node[0]))
           
    def close_windows(self):
        self.master.destroy()        

class SessionWindow:
    def __init__(self, master):
        self.master=master
        
        self.SessionFrame=ttk.Frame(self.master)
        
        self.ActiveSessionFrame=ttk.LabelFrame(self.SessionFrame, text="Active Session",  padding="10 10 12 12")
        self.ActiveSessionFrame.grid(column=1, row=1)
        self.ActiveSessionInfoFrame=ttk.LabelFrame(self.ActiveSessionFrame, text="Active Session Info", padding="10 10 12 12")
        self.ActiveSessionInfoFrame.grid(column=2, row=1)
        ttk.Label(self.ActiveSessionInfoFrame, text="Node ID Number ").grid(row=1 , column=1)
        ttk.Label(self.ActiveSessionInfoFrame, text="Session Number ").grid(row=1 , column=2)
        ttk.Label(self.ActiveSessionInfoFrame, text="Timing Epoch ").grid(row=1 , column=3)
        ttk.Label(self.ActiveSessionInfoFrame, text="Start of Last Session ").grid(row=1 , column=4)
        ttk.Label(self.ActiveSessionInfoFrame, text="Start of Next Session ").grid(row=2 , column=1)
        ttk.Label(self.ActiveSessionInfoFrame, text="Length of Current Session ").grid(row=2 , column=2)
        ttk.Label(self.ActiveSessionInfoFrame, text="Sampling Rate ").grid(row=2 , column=3)
        ttk.Label(self.ActiveSessionInfoFrame, text="Start Time").grid(row=2 , column=4)
        ttk.Label(self.ActiveSessionInfoFrame, text="End Time").grid(row=3, column=1)
        ttk.Label(self.ActiveSessionInfoFrame, text="Axis Settings ").grid(row=3, column=2)
        ttk.Label(self.ActiveSessionInfoFrame, text="Session Status ").grid(row=3 , column=3)
        ttk.Label(self.ActiveSessionInfoFrame, text="Session Notes: ").grid(row=3 , column=4)
        self.ActiveSessionHex=tk.Text(self.ActiveSessionFrame)
        self.ActiveSessionHex.grid(row=3, column=1)
        ttk.Label(self.ActiveSessionFrame, text="Raw Hex Data").grid(row=2, column=1)
        self.ActiveSessionParsedAscii=tk.Text(self.ActiveSessionFrame)
        self.ActiveSessionParsedAscii.grid(row=3, column=3)
        ttk.Label(self.ActiveSessionFrame, text="Parsed ASCII Data").grid(row=2, column=3)
        
        self.middleFrame=ttk.Frame(self.ActiveSessionFrame)
        self.middleFrame.grid(row=3, column=2)
        ttk.Label(self.middleFrame, text="Parsed CRC").grid(column=1, row=1)
        ttk.Label(self.middleFrame, text="Calculated CRC").grid(column=1, row=2)
        self.ParsedCRC=tk.StringVar()
        self.ParsedCRC.set('JKASDAD')
        self.PCRCLABEL=ttk.Label(self.middleFrame, textvariable=self.ParsedCRC).grid(column=2, row=1)
        self.CalculatedCRC=tk.StringVar().set('ABDAHSDA')
        self.CCRCLABEL=ttk.Label(self.middleFrame, textvariable=self.CalculatedCRC).grid(column=2, row=2)
        self.SessionFrame.pack()
        
        

app=Application(root)

root.mainloop()
