import struct
import traceback
import time

from google.protobuf import symbol_database as db
from antirobot.idl import antirobot_ev_pb2 as ev_pb2

EV_HEADER_FORMAT = "=IQI"
EV_HEADER_SIZE = struct.calcsize(EV_HEADER_FORMAT)


def GetProtobufMessageClasses():
    return db.Default().GetMessages(['antirobot/idl/antirobot.ev'])


def GetAntirobotEventClasses():
    return [x for x in GetProtobufMessageClasses().itervalues() if x is not ev_pb2.THeader]


def GetMsgClsMessageId(msgCls):
    return int(str(msgCls.DESCRIPTOR.GetOptions()).split(':')[1])  # FIXME: change to something smarter


ID_TO_EV = dict(
    (GetMsgClsMessageId(evCls), evCls)
    for evName, evCls in GetProtobufMessageClasses().items()
    if evName not in ['NAntirobotEvClass.THeader', 'NAntirobotEvClass.TProtoseqRecord']
    )
EV_TO_ID = dict((y.__name__, x) for (x, y) in ID_TO_EV.items())


class AntirobotEvent:
    def __init__(self, msgClassId, msg, timestamp):
        self.EventClassId = msgClassId
        self.EventType = msg.__class__.__name__
        self.Event = msg
        self.Timestamp = timestamp

    # Create AntirobotEvent from scratch
    # params is a dict containing values for event fields
    @staticmethod
    def Create(msgCls, timestamp=time.time()*1000000, params={}):
        def AssignParams(obj, aDict):
            for k, v in aDict.items():
                field = getattr(obj, k)
                if isinstance(v, dict):
                    AssignParams(field, v)

                elif isinstance(v, list):
                    field.extend(v)
                else:
                    setattr(obj, k, v)

        pbMsg = msgCls()
        AssignParams(pbMsg, params)

        return AntirobotEvent(EV_TO_ID[msgCls.__name__], pbMsg, timestamp)

    @staticmethod
    def CreateFromString(binStr, acceptableClasses=None):
        (evLen, ts, evClass) = struct.unpack(EV_HEADER_FORMAT, binStr[:EV_HEADER_SIZE])

        if acceptableClasses and evClass not in acceptableClasses:
            return None

        try:
            if evClass not in ID_TO_EV:
                raise Exception("invalid evClass %d\t%s" % (evClass, ','.join(["%x" % ord(c) for c in binStr[:EV_HEADER_SIZE]])))

            pbMsg = ID_TO_EV[evClass]()
        except:
            traceback.print_exc()
            return None

        try:
            pbMsg.ParseFromString(binStr[EV_HEADER_SIZE:])

            return AntirobotEvent(evClass, pbMsg, ts)
        except:
            traceback.print_exc()
            return None

    def SerializeToString(self):
        evBinStr = self.Event.SerializeToString()
        evLen = EV_HEADER_SIZE + len(evBinStr)
        header = struct.pack(EV_HEADER_FORMAT, evLen, self.Timestamp, self.EventClassId)

        return header + evBinStr

    def __getstate__(self):
        return {
            'EventClassId': self.EventClassId,
            'EventType': self.EventType,
            'Timestamp': self.Timestamp,
            'Event': self.Event.SerializeToString()
            }

    def __setstate__(self, state):
        self.EventClassId = state['EventClassId']
        self.EventType = state['EventType']
        self.Timestamp = state['Timestamp']
        self.Event = ID_TO_EV[state['EventClassId']]()
        self.Event.ParseFromString(state['Event'])


def Event(binEvent, acceptableClasses=None):
    return AntirobotEvent.CreateFromString(binEvent, acceptableClasses)


def GetMsgClassByName(clsName):
    return ID_TO_EV[EV_TO_ID[clsName]]
