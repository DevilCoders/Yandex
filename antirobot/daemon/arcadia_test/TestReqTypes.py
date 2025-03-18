from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import GenRandomIP, Fullreq

import time


IP_HEADER = "X-Forwarded-For-Y"
MATRIXNET_FOR_IGNORED_REQTYPES = -123
CBB_FLAG_CAN_SHOW_CAPTCHA = 656
CBB_SYNC_PERIOD = 1


class TestReqTypes(AntirobotTestSuite):
    options = {
        "re_queries": [
            {"path": "/analytical", "req_type": "analytical"},
            {"path": "/search", "req_type": "ys"},
            {"path": "/garbage", "req_type": "other"},
        ],
        "NoMatrixnetReqTypes": "other",
        "MatrixnetResultForNoMatrixnetReqTypes": MATRIXNET_FOR_IGNORED_REQTYPES,
        "cbb_can_show_captcha_flag": CBB_FLAG_CAN_SHOW_CAPTCHA,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
    }

    def create_request(self, custom_text):
        ip = GenRandomIP()
        return Fullreq("http://yandex.ru" + custom_text, headers={IP_HEADER: ip}), ip

    def test_req_type_other_matrixnet_not_calculated(self):
        request, ip = self.create_request("/garbage")

        self.antirobot.send_request(request)
        log_entry = self.unified_agent.get_last_event_in_daemon_logs(ip)
        assert log_entry["req_type"] == "web-other"
        assert log_entry["matrixnet"] == MATRIXNET_FOR_IGNORED_REQTYPES

        self.cbb.add_text_block(CBB_FLAG_CAN_SHOW_CAPTCHA, "doc=/.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        self.antirobot.send_request(request)
        log_entry = self.unified_agent.get_last_event_in_daemon_logs(ip)
        assert log_entry["req_type"] == "web-other"
        assert log_entry["matrixnet"] != MATRIXNET_FOR_IGNORED_REQTYPES

    def test_reqtype_search_matrixnet_calculated(self):
        request, ip = self.create_request("/search")
        self.antirobot.send_request(request)

        log_entry = self.unified_agent.get_last_event_in_daemon_logs(ip)

        assert log_entry["req_type"] == "web-ys"
        assert log_entry["matrixnet"] != MATRIXNET_FOR_IGNORED_REQTYPES

    def test_req_type_analytical_matrixnet_calculated(self):
        request, ip = self.create_request("/analytical")
        self.antirobot.send_request(request)

        log_entry = self.unified_agent.get_last_event_in_daemon_logs(ip)

        assert log_entry["req_type"] == "web-analytical"
        assert not log_entry["may_ban"]
        assert not log_entry["can_show_captcha"]
        assert log_entry["matrixnet"] != MATRIXNET_FOR_IGNORED_REQTYPES
