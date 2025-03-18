import time
from pathlib import Path

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    IsCaptchaRedirect,
    IsBlockedResponse,
)

BAN_SOURCE_IP_HEADER = "X-Antirobot-Ban-Source-Ip"
IP_HEADER = "X-Forwarded-For-Y"
CBB_SYNC_PERIOD = 0.1
CBB_FLAG_BAN_SOURCE_IP = 808
CBB_FLAG_CAPTCHA_BY_REGEXP = 13


class TestBanSourceIpHeader(AntirobotTestSuite):
    DataDir = Path.cwd()
    Whitelist = DataDir / "whitelist_ips"
    WhitelistIpsAll = DataDir / "whitelist_ips_all"

    options = {
        "WhiteList": Whitelist.name,
        "WhitelistsDir": DataDir,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbFlagBanSourceIp": CBB_FLAG_BAN_SOURCE_IP,
        "cbb_captcha_re_flag": CBB_FLAG_CAPTCHA_BY_REGEXP,
        "DisableBansByFactors": 1,
    }

    @classmethod
    def setup_class(cls):
        for f in [cls.Whitelist, cls.WhitelistIpsAll]:
            open(f, "w").close()
        super().setup_class()

    def get_full_req(self, req, ip, spravka=None):
        headers = {'X-Forwarded-For-Y': ip}
        if spravka:
            headers['Cookie'] = 'spravka=%s' % spravka

        return Fullreq(req, headers=headers)

    def get_search_req(self, ip, text, spravka, url="https://yandex.ru/yandsearch"):
        return self.get_full_req(f"{url}?text={text}", ip, spravka)

    def get_metric(self):
        return self.antirobot.query_metric(
            "ban_source_ip_deee",
            service_type="web",
        )

    def test_ban_source_ip_header(self):
        ip = '22.13.42.2'
        whitelist_ip = '22.13.42.3'

        with open(self.Whitelist, "a") as f:
            f.write(whitelist_ip + "\n")

        self.antirobot.reload_data()

        metric_cnt = self.get_metric()

        resp = self.send_request(self.get_search_req(ip, "cats", None))
        assert BAN_SOURCE_IP_HEADER not in resp.headers

        ban_source_ip = self.unified_agent.wait_log_line_with_query('.*cats.*')["ban_source_ip"]
        assert ban_source_ip is False
        ban_source_ip = self.unified_agent.wait_cacher_log_line_with_query('.*cats.*').ban_source_ip
        assert ban_source_ip is False

        assert self.get_metric() == metric_cnt

        self.cbb.add_text_block(
            CBB_FLAG_BAN_SOURCE_IP, "cgi=/.*cats3.*/"
        )
        time.sleep(CBB_SYNC_PERIOD + 1)

        resp = self.send_request(self.get_search_req(ip, "cats3", None))
        assert resp.headers[BAN_SOURCE_IP_HEADER] == "1"

        assert self.get_metric() == metric_cnt + 1
        metric_cnt = self.get_metric()

        ban_source_ip = self.unified_agent.wait_log_line_with_query('.*cats3.*')["ban_source_ip"]
        assert ban_source_ip is True
        ban_source_ip = self.unified_agent.wait_cacher_log_line_with_query('.*cats3.*').ban_source_ip
        assert ban_source_ip is True

        resp = self.send_request(self.get_search_req(whitelist_ip, "cats3", None))
        assert BAN_SOURCE_IP_HEADER not in resp.headers

        assert self.get_metric() == metric_cnt

        ban_source_ip = self.unified_agent.wait_log_line_with_query('.*cats3.*')["ban_source_ip"]
        assert ban_source_ip is False
        ban_source_ip = self.unified_agent.wait_cacher_log_line_with_query('.*cats3.*').ban_source_ip
        assert ban_source_ip is False

    def test_ban_source_ip_with_captcha(self):
        ip = '22.13.42.2'

        metric_cnt = self.get_metric()

        resp = self.send_request(self.get_search_req(ip, "cats", None))
        assert BAN_SOURCE_IP_HEADER not in resp.headers
        ban_source_ip = self.unified_agent.wait_log_line_with_query('.*cats.*')["ban_source_ip"]
        assert ban_source_ip is False
        ban_source_ip = self.unified_agent.wait_cacher_log_line_with_query('.*cats.*').ban_source_ip
        assert ban_source_ip is False
        assert self.get_metric() == metric_cnt

        self.cbb.add_text_block(
            CBB_FLAG_BAN_SOURCE_IP, "cgi=/.*cats3.*/"
        )
        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*cats3.*/")

        time.sleep(CBB_SYNC_PERIOD + 1)

        resp = self.send_request(self.get_search_req(ip, "cats3", None))
        assert IsCaptchaRedirect(resp)
        assert resp.headers[BAN_SOURCE_IP_HEADER] == "1"
        assert self.get_metric() == metric_cnt + 1
        ban_source_ip = self.unified_agent.wait_log_line_with_query('.*cats3.*')["ban_source_ip"]
        assert ban_source_ip is True
        ban_source_ip = self.unified_agent.wait_cacher_log_line_with_query('.*cats3.*').ban_source_ip
        assert ban_source_ip is True

    def test_ban_source_ip_with_block(self):
        ip = '22.13.42.2'

        self.antirobot.block(ip)
        self.cbb.add_text_block(
            CBB_FLAG_BAN_SOURCE_IP, "cgi=/.*cats3.*/"
        )
        time.sleep(CBB_SYNC_PERIOD + 1)

        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "cats3", None))
        assert IsBlockedResponse(resp)
        assert resp.headers[BAN_SOURCE_IP_HEADER] == "1"
        assert self.get_metric() == metric_cnt + 1
        ban_source_ip = self.unified_agent.wait_log_line_with_query('.*cats3.*')["ban_source_ip"]
        assert ban_source_ip is True
        ban_source_ip = self.unified_agent.wait_cacher_log_line_with_query('.*cats3.*').ban_source_ip
        assert ban_source_ip is True
