from google.protobuf import symbol_database
from antirobot.idl import daemon_log_pb2
from google.protobuf.json_format import MessageToDict, MessageToJson


def GetProtobufMessageClasses():
    return symbol_database.Default().GetMessages(['antirobot/idl/daemon_log.proto'])


def GetAntirobotTProcessorRecordClasses():
    return [x for x in GetProtobufMessageClasses().itervalues() if x is not daemon_log_pb2.TProcessorRecord]


class AntirobotCacherDaemonLog:
    def __init__(self, pbMsg):
        self.PbMsg = pbMsg

        self.unique_key = pbMsg.unique_key
        self.timestamp = pbMsg.timestamp
        self.verdict = pbMsg.verdict
        self.req_type = pbMsg.req_type
        self.balancer_ip = pbMsg.balancer_ip
        self.spravka_ip = pbMsg.spravka_ip
        self.ident_type = pbMsg.ident_type
        self.user_ip = pbMsg.user_ip
        self.req_id = pbMsg.req_id
        self.random_captcha = pbMsg.random_captcha
        self.partner_ip = pbMsg.partner_ip
        self.yandexuid = pbMsg.yandexuid
        self.cacher_host = pbMsg.cacher_host
        self.icookie = pbMsg.icookie
        self.service = pbMsg.service
        self.may_ban = pbMsg.may_ban
        self.can_show_captcha = pbMsg.can_show_captcha
        self.req_url = pbMsg.req_url
        self.ip_list = pbMsg.ip_list
        self.block_reason = pbMsg.block_reason
        self.cacher_blocked = pbMsg.cacher_blocked
        self.cacher_block_reason = pbMsg.cacher_block_reason
        self.ja3 = pbMsg.ja3
        self.robotness = pbMsg.robotness
        self.host = pbMsg.host
        self.suspiciousness = pbMsg.suspiciousness
        self.user_agent = pbMsg.user_agent
        self.referer = pbMsg.referer
        self.cbb_whitelist = pbMsg.cbb_whitelist
        self.ja4 = pbMsg.ja4
        self.req_group = pbMsg.req_group
        self.cbb_mark_rules = pbMsg.cbb_mark_rules
        self.cbb_ban_rules = pbMsg.cbb_ban_rules
        self.spravka_ignored = pbMsg.spravka_ignored
        self.catboost_whitelist = pbMsg.catboost_whitelist
        self.cbb_checkbox_blacklist_rules = pbMsg.cbb_checkbox_blacklist_rules
        self.cbb_can_show_captcha_rules = pbMsg.cbb_can_show_captcha_rules
        self.degradation = pbMsg.degradation
        self.p0f = pbMsg.p0f
        self.experiments_test_id = pbMsg.experiments_test_id
        self.hodor = pbMsg.hodor
        self.hodor_hash = pbMsg.hodor_hash
        self.lcookie = pbMsg.lcookie
        self.jws_state = pbMsg.jws_state
        self.cacher_formula_result = pbMsg.cacher_formula_result
        self.ban_source_ip = pbMsg.ban_source_ip
        self.mini_geobase_mask = pbMsg.mini_geobase_mask
        self.jws_hash = pbMsg.jws_hash
        self.yandex_trust_state = pbMsg.yandex_trust_state
        self.valid_spravka_hash = pbMsg.valid_spravka_hash
        self.valid_autoru_tamper = pbMsg.valid_autoru_tamper
        self.ban_fw_source_ip = pbMsg.ban_fw_source_ip

    def GetDict(self):
        return MessageToDict(self.PbMsg, preserving_proto_field_name=True)

    def GetJson(self):
        return MessageToJson(self.PbMsg, preserving_proto_field_name=True)

    def SerializeToString(self):
        return self.PbMsg.SerializeToString()


def CacherDaemonLog(pbMsg):
    """
        Input: 'pbMsg' - protobuf object, that is serialized as string

        Initialize AntirobotCacherDaemonLog object,
        that has 'PbMsg' field
        and all fields representing TCacherRecord

        Return: AntirobotCacherDaemonLog object
    """
    return AntirobotCacherDaemonLog(pbMsg)
