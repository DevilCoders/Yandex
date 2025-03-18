import time

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import Fullreq

DEGRADATION_HEADER = "X-Yandex-Antirobot-Degradation"
IP_HEADER = "X-Forwarded-For-Y"
CBB_SYNC_PERIOD = 0.1
CBB_FLAG_DEGRADATION = 661
PRIVILEGED_IP = "42.42.42.42"
NOT_PRIVILEGED_IP = "42.42.42.13"
WATCHER_PERIOD = 0.1


class TestDegradationHeader(AntirobotTestSuite):
    options = {
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbFlagDegradation": CBB_FLAG_DEGRADATION,
        "DisableBansByFactors": 1,
        "HandleWatcherPollInterval": WATCHER_PERIOD,
    }

    @classmethod
    def setup_class(cls):
        super().setup_class()

    def get_full_req(self, req, ip, spravka=None):
        headers = {'X-Forwarded-For-Y': ip}
        if spravka:
            headers['Cookie'] = 'spravka=%s' % spravka

        return Fullreq(req, headers=headers)

    def get_search_req(self, ip, text, spravka, url="https://yandex.ru/yandsearch"):
        return self.get_full_req(f"{url}?text={text}", ip, spravka)

    def gen_spravka(self):
        "Returns a generated spravka with degradation on web"

        resp = self.send_request("/admin?action=getspravka&domain=yandex.ru&degradation=web")
        return resp.read().decode().strip()

    def get_metric(self):
        return self.antirobot.query_metric(
            "with_degradation_requests_deee",
            service_type="web",
        )

    def test_degradation_header(self):
        ip = '22.13.42.2'
        spravka = self.gen_spravka()

        metric_cnt = self.get_metric()

        resp = self.send_request(self.get_search_req(ip, "cats", None))
        assert resp.headers[DEGRADATION_HEADER] == "0"

        degradation = self.unified_agent.wait_log_line_with_query('.*cats.*')["degradation"]
        assert degradation is False

        resp = self.send_request(self.get_search_req(ip, "cats2", spravka))
        assert resp.headers[DEGRADATION_HEADER] == "0"

        degradation = self.unified_agent.wait_log_line_with_query('.*cats2.*')["degradation"]
        assert degradation is False

        assert self.get_metric() == metric_cnt

        self.cbb.add_text_block(
            CBB_FLAG_DEGRADATION, "degradation=yes"
        )
        time.sleep(CBB_SYNC_PERIOD + 1)

        resp = self.send_request(self.get_search_req(ip, "cats3", spravka))
        assert resp.headers[DEGRADATION_HEADER] == "1"

        assert self.get_metric() == metric_cnt + 1

        degradation = self.unified_agent.wait_log_line_with_query('.*cats3.*')["degradation"]
        assert degradation is True
