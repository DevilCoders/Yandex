from google.protobuf import symbol_database
from antirobot.idl import daemon_log_pb2
from google.protobuf.json_format import MessageToDict, MessageToJson


def GetProtobufMessageClasses():
    return symbol_database.Default().GetMessages(['antirobot/idl/daemon_log.proto'])


def GetAntirobotTCacherRecordClasses():
    return [x for x in GetProtobufMessageClasses().itervalues() if x is not daemon_log_pb2.TCacherRecord]


class AntirobotProcessorDaemonLog:
    def __init__(self, pbMsg):
        self.PbMsg = pbMsg

        self.unique_key = pbMsg.unique_key
        self.timestamp = pbMsg.timestamp
        self.matrixnet = pbMsg.matrixnet
        self.web_req_count = pbMsg.web_req_count
        self.wizard_error = pbMsg.wizard_error
        self.processor_host = pbMsg.processor_host
        self.missed = pbMsg.missed
        self.threshold = pbMsg.threshold
        self.exp_formulas = pbMsg.exp_formulas
        self.rules = pbMsg.rules
        self.mark_rules = pbMsg.mark_rules
        self.prev_rules = pbMsg.prev_rules
        self.prev_cbb_ban_rules = pbMsg.prev_cbb_ban_rules
        self.force_ban = pbMsg.force_ban
        self.cacher_exp_formulas = pbMsg.cacher_exp_formulas
        self.matrixnet_fallback = pbMsg.matrixnet_fallback

    def GetDict(self):
        return MessageToDict(self.PbMsg, preserving_proto_field_name=True)

    def GetJson(self):
        return MessageToJson(self.PbMsg, preserving_proto_field_name=True)

    def SerializeToString(self):
        return self.PbMsg.SerializeToString()


def ProcessorDaemonLog(pbMsg):
    """
        Input: 'pbMsg' - protobuf object, that is serialized as string

        Initialize AntirobotProcessorDaemonLog object,
        that has 'PbMsg' field
        and all fields representing TCacherRecord

        Return: AntirobotProcessorDaemonLog object
    """
    return AntirobotProcessorDaemonLog(pbMsg)
