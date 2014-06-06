'''
Created on Mar 31, 2014

@author: Bill Devine
'''

import struct
import datetime
import time
import ftdi3
from fletcher import fletcher16
import threading
import asyncutil
from collections import namedtuple

CardInfoSample=b'ABCDEFGHIJKLMNOPQRSTUVWXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'

class Filesystem:
    def __init__(self):
        nodeIDFormat='H'
        cardStatusFormat='B'
        epochFormat='H'
        lastDataFormat='I'
        lastSessFormat=lastDataFormat
        startSectorFormat=lastDataFormat
        timeFormat='12s'
        notesFormat='140s'
        self.CardInfoSample=b'ABCDEFGHIJKLMNOPQRSTUVWXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'

        
        self.ActiveCardInfoSector=namedtuple('CardInfoSector', 'NodeIDNumber CardStatus TimingEpoch LastDataSectorWritten LastSessionInfoSectorWritten StartSector CardInitializationTime Notes', verbose='true' )
        self.ActiveSessionInfoSector=namedtuple('SessionInfoSector', 'SessionNumber NodeIDNumber TimingEpoch StartOfLastSession StartOfNextSession LengthOfCurrentSession SamplingRate StartTime EndTime AxisSettings SessionStatus Notes', verbose='true')
        
        serialFormat=nodeIDFormat
        lastSessSectorFormat=lastDataFormat
        nextSessSectorFormat=lastDataFormat
        lengthFormat=lastDataFormat
        samplingRateFormat=nodeIDFormat
        axisCtrlFormat='B'
        sessStatusFormat='B'
        
        self.CardInfoSectorFormat='<'+nodeIDFormat+cardStatusFormat+epochFormat+lastDataFormat+lastSessFormat+startSectorFormat+timeFormat+notesFormat
        self.SessionInfoSectorFormat='<'+serialFormat+nodeIDFormat+epochFormat+lastSessSectorFormat+nextSessSectorFormat+lengthFormat+samplingRateFormat+timeFormat+timeFormat+axisCtrlFormat+sessStatusFormat+notesFormat
    def parseCardInfo(self, CardInfoSector):
        self.ActiveCardInfoSector._make(struct.unpack(self.CardInfoSectorFormat, CardInfoSector))
    def parseSessInfo(self, SessInfoSector):
        self.ActiveSessionInfoSector._make(struct.unpack(self.SessionInfoSectorFormat, SessInfoSector))
        
        