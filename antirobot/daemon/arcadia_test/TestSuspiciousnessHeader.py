import time
import pytest
from pathlib import Path

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util.asserts import AssertBlocked
from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    IsCaptchaRedirect,
    IsBlockedResponse,
)
from antirobot.daemon.arcadia_test.util.spravka import GetSpravkaForAddr


SUSPECTED_ROBOT_HEADER = "X-Yandex-Suspected-Robot"
SUSPICIOUSNESS_HEADER = "X-Antirobot-Suspiciousness-Y"
IP_HEADER = "X-Forwarded-For-Y"
CBB_SYNC_PERIOD = 0.1
CBB_FLAG_SUSPICIOUSNESS = 511
PRIVILEGED_IP = "42.42.42.42"
NOT_PRIVILEGED_IP = "42.42.42.13"
WATCHER_PERIOD = 0.1


class TestSuspiciousnessHeader(AntirobotTestSuite):
    privileged_ips_file = str(Path.cwd() / "privileged_ips")
    controls = Path.cwd() / "controls"
    many_requests_file = str(controls / "suspicious_429")
    many_requests_mobile_file = str(controls / "suspicious_mobile_429")
    ban_file = str(controls / "suspicious_ban")
    block_file = str(controls / "suspicious_block")
    options = {
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbFlagSuspiciousness": CBB_FLAG_SUSPICIOUSNESS,
        "DisableBansByFactors": 1,
        "suspiciousness_header_enabled@collections": False,
        "suspiciousness_header_enabled@web": True,
        "suspiciousness_header_enabled@img": False,
        "PrivilegedIpsFile": privileged_ips_file,
        "HandleManyRequestsEnableServicePath": many_requests_file,
        "HandleManyRequestsMobileEnableServicePath": many_requests_mobile_file,
        "HandleSuspiciousBanServicePath" : ban_file,
        "HandleSuspiciousBlockServicePath" : block_file,
        "HandleWatcherPollInterval": WATCHER_PERIOD,
    }

    captcha_args = ['--generate', 'correct', '--check', 'success']

    @classmethod
    def setup_class(cls):
        cls.controls.mkdir(exist_ok=True)
        with open(cls.privileged_ips_file, "w") as f:
            print(PRIVILEGED_IP, file=f)
        super().setup_class()

    def setup_method(self, method):
        super().setup_method(method)
        for fname in [self.many_requests_file, self.ban_file, self.block_file]:
            with open(fname, "w") as f:
                f.write("")

    def get_full_req(self, req, ip, spravka=None):
        headers = {'X-Forwarded-For-Y': ip}
        if spravka:
            headers['Cookie'] = 'spravka=%s' % spravka

        return Fullreq(req, headers=headers)

    def get_search_req(self, ip, text, spravka, url="https://yandex.ru/yandsearch"):
        return self.get_full_req(f"{url}?text={text}", ip, spravka)

    def get_mobile_search_req(self, ip, text, spravka, url="https://m.yandex.ru/yandsearch"):
        return self.get_full_req(f"{url}?text={text}", ip, spravka)

    def get_metric(self):
        return self.antirobot.query_metric(
            "with_suspiciousness_requests_deee",
            service_type="web",
        )

    def get_metric_many_requests(self):
        return self.antirobot.query_metric(
            "many_requests_deee",
            service_type="web",
        )

    def get_metric_many_requests_mobile(self):
        return self.antirobot.query_metric(
            "many_requests_mobile_deee",
            service_type="web",
        )

    def ban_spravka(self):
        self.cbb.add_text_block(
            CBB_FLAG_SUSPICIOUSNESS, "ident_type=/2-.*/"
        )
        time.sleep(CBB_SYNC_PERIOD + 0.1)

    def test_suspected_robot_header(self):
        ip = '22.13.42.2'
        spravka = GetSpravkaForAddr(self, ip)

        resp = self.send_request(self.get_search_req(ip, "cats", None))
        assert resp.headers[SUSPECTED_ROBOT_HEADER] == "0"

        resp = self.send_request(self.get_search_req(ip, "cats", "invalid_spravka"))
        assert resp.headers[SUSPECTED_ROBOT_HEADER] == "0"

        resp = self.send_request(self.get_search_req(ip, "cats", spravka))
        assert resp.headers[SUSPECTED_ROBOT_HEADER] == "1"

    def test_suspiciousness_header(self):
        ip = '22.13.42.3'
        spravka = GetSpravkaForAddr(self, ip)

        self.unified_agent.pop_daemon_logs()

        metric_cnt = self.get_metric()

        resp = self.send_request(self.get_search_req(ip, "cats_1", None))
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert self.unified_agent.wait_log_line_with_query(r".*cats_1.*")["suspiciousness"] == 0.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_1.*").suspiciousness == 0.0
        assert self.get_metric() == metric_cnt

        resp = self.send_request(self.get_search_req(ip, "cats_2", "invalid_spravka"))
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert self.unified_agent.wait_log_line_with_query(r".*cats_2.*")["suspiciousness"] == 0.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_2.*").suspiciousness == 0.0
        assert self.get_metric() == metric_cnt

        # spravka is not set SUSPICIOUSNESS_HEADER
        resp = self.send_request(self.get_search_req(ip, "cats_3", spravka))
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert self.unified_agent.wait_log_line_with_query(r".*cats_3.*")["suspiciousness"] == 0.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_3.*").suspiciousness == 0.0
        assert self.get_metric() == metric_cnt

        self.ban_spravka()

        # now spravka is set SUSPICIOUSNESS_HEADER
        resp = self.send_request(self.get_search_req(ip, "cats_4", spravka))
        assert resp.headers[SUSPICIOUSNESS_HEADER] == '1.0'
        assert self.unified_agent.wait_log_line_with_query(r".*cats_4.*")["suspiciousness"] == 1.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_4.*").suspiciousness == 1.0
        assert self.get_metric() == metric_cnt + 1

    @pytest.mark.parametrize('ip, is_whitelisted', [
        (NOT_PRIVILEGED_IP, False),
        (PRIVILEGED_IP, True),
    ])
    def test_whitelisted(self, ip, is_whitelisted):
        spravka = GetSpravkaForAddr(self, ip)

        resp = self.send_request(self.get_search_req(ip, "hallo", spravka))
        assert resp.getcode() == 200
        if is_whitelisted:
            assert SUSPICIOUSNESS_HEADER not in resp.headers
        else:
            assert resp.headers[SUSPICIOUSNESS_HEADER] == '1.0'

    def test_blocked_by_cbb_by_regexp(self):
        ip = '22.13.42.4'
        self.cbb.add_text_block(
            CBB_FLAG_SUSPICIOUSNESS, "cgi=/.*suspected_robot.*/"
        )
        time.sleep(CBB_SYNC_PERIOD + 0.1)

        robot_request = Fullreq(
            "http://yandex.ru/search?text=i_am_suspected_robot",
            headers={IP_HEADER: ip},
        )

        metric_cnt = self.get_metric()

        assert self.send_request(robot_request).headers[SUSPICIOUSNESS_HEADER] == "1.0"
        assert self.get_metric() == metric_cnt + 1

        not_robot_request = Fullreq(
            "http://yandex.ru/search?text=i_am_not_robot",
            headers={IP_HEADER: ip},
        )

        assert self.send_request(not_robot_request).headers.get(SUSPICIOUSNESS_HEADER, "0.0") == "0.0"
        assert self.get_metric() == metric_cnt + 1

    def test_not_passed_to_user(self):
        ip = '22.13.42.5'
        spravka = GetSpravkaForAddr(self, ip)

        self.ban_spravka()

        resp = self.send_request(self.get_search_req(ip, "cats", spravka))
        assert resp.headers[SUSPICIOUSNESS_HEADER] == "1.0"

        self.antirobot.block(ip)
        resp = self.send_request(self.get_search_req(ip, "cats", spravka))
        AssertBlocked(resp)
        assert SUSPICIOUSNESS_HEADER not in resp.headers

    @pytest.mark.parametrize('url, expected_value', [
        ("http://images.yandex.ru/search", "0.0"),
        ("http://yandex.ru/search", "1.0"),
        ("http://yandex.ru/collections", "0.0"),
    ])
    def test_disabled_in_config(self, url, expected_value):
        ip = '22.13.42.6'
        spravka = GetSpravkaForAddr(self, ip)

        resp = self.send_request(self.get_search_req(ip, "cats", spravka, url))
        assert resp.headers.get(SUSPICIOUSNESS_HEADER, '0.0') == expected_value

    def test_suspiciousness_429(self):
        ip = '22.13.42.7'
        spravka = GetSpravkaForAddr(self, ip)

        self.ban_spravka()

        self.unified_agent.pop_daemon_logs()

        metric_cnt = self.get_metric()
        many_requests_cnt = self.get_metric_many_requests()

        resp = self.send_request(self.get_search_req(ip, "cats_1", None))
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert self.unified_agent.wait_log_line_with_query(r".*cats_1.*")["suspiciousness"] == 0.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_1.*").suspiciousness == 0.0
        assert self.get_metric() == metric_cnt
        assert self.get_metric_many_requests() == many_requests_cnt

        resp = self.send_request(self.get_search_req(ip, "cats_2", "invalid_spravka"))
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert self.unified_agent.wait_log_line_with_query(r".*cats_2.*")["suspiciousness"] == 0.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_2.*").suspiciousness == 0.0
        assert self.get_metric() == metric_cnt
        assert self.get_metric_many_requests() == many_requests_cnt

        resp = self.send_request(self.get_search_req(ip, "cats_3", spravka))
        assert resp.headers[SUSPICIOUSNESS_HEADER] == "1.0"
        assert self.unified_agent.wait_log_line_with_query(r".*cats_3.*")["suspiciousness"] == 1.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_3.*").suspiciousness == 1.0
        assert self.get_metric() == metric_cnt + 1
        assert self.get_metric_many_requests() == many_requests_cnt

        with open(self.many_requests_file, "w") as f:
            f.write("web")
        time.sleep(WATCHER_PERIOD + 0.1)

        # suspiciousness request - 429
        metric_cnt = self.get_metric()
        many_requests_cnt = self.get_metric_many_requests()
        many_requests_mobile_cnt = self.get_metric_many_requests_mobile()
        resp = self.send_request(self.get_search_req(ip, "cats_4", spravka))

        content = resp.read().decode()
        needLog = self.unified_agent.wait_log_line_with_query(r".*cats_4.*")
        strKey = str(needLog["unique_key"])

        params = [
            f'"unique_key": "{strKey}"'
            ]

        for substring in params:
            assert substring in content

        assert resp.getcode() == 429
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert resp.headers["Retry-After"] == "600"
        assert needLog["suspiciousness"] == 1.0
        assert self.get_metric() == metric_cnt + 1
        assert self.get_metric_many_requests() == many_requests_cnt + 1
        assert self.get_metric_many_requests_mobile() == many_requests_mobile_cnt

        # non suspiciousness request 200
        many_requests_cnt = self.get_metric_many_requests()
        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "cats_5", "invalid_spravka"))
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert self.unified_agent.wait_log_line_with_query(r".*cats_5.*")["suspiciousness"] == 0.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_5.*").suspiciousness == 0.0
        assert self.get_metric() == metric_cnt
        assert self.get_metric_many_requests() == many_requests_cnt

        # suspiciousness request to mobile - 200
        many_requests_cnt = self.get_metric_many_requests()
        many_requests_mobile_cnt = self.get_metric_many_requests_mobile()
        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_mobile_search_req(ip, "cats_7", spravka))
        assert resp.getcode() == 200
        assert self.unified_agent.wait_log_line_with_query(r".*cats_7.*")["suspiciousness"] == 1.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_7.*").suspiciousness == 1.0
        assert self.get_metric() == metric_cnt + 1
        assert self.get_metric_many_requests() == many_requests_cnt
        assert self.get_metric_many_requests_mobile() == many_requests_mobile_cnt

        # disable many requests on desktop enable on mobile
        with open(self.many_requests_file, "w") as f:
            f.write("")
        with open(self.many_requests_mobile_file, "w") as f:
            f.write("web")
        time.sleep(WATCHER_PERIOD + 0.1)

        # suspiciousness request to mobile - 429
        metric_cnt = self.get_metric()
        many_requests_cnt = self.get_metric_many_requests()
        many_requests_mobile_cnt = self.get_metric_many_requests_mobile()
        resp = self.send_request(self.get_mobile_search_req(ip, "cats_8", spravka))
        assert resp.getcode() == 429
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert resp.headers["Retry-After"] == "600"
        assert self.unified_agent.wait_log_line_with_query(r".*cats_8.*")["suspiciousness"] == 1.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_8.*").suspiciousness == 1.0
        assert self.get_metric() == metric_cnt + 1
        assert self.get_metric_many_requests() == many_requests_cnt
        assert self.get_metric_many_requests_mobile() == many_requests_mobile_cnt + 1

        # non suspiciousness request to mobile - 200
        many_requests_mobile_cnt = self.get_metric_many_requests_mobile()
        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_mobile_search_req(ip, "cats_9", "invalid_spravka"))
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert self.unified_agent.wait_log_line_with_query(r".*cats_9.*")["suspiciousness"] == 0.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_9.*").suspiciousness == 0.0
        assert self.get_metric() == metric_cnt
        assert self.get_metric_many_requests_mobile() == many_requests_mobile_cnt

        # suspiciousness request - 200
        many_requests_cnt = self.get_metric_many_requests()
        many_requests_mobile_cnt = self.get_metric_many_requests_mobile()
        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "cats_10", spravka))
        assert resp.getcode() == 200
        assert self.unified_agent.wait_log_line_with_query(r".*cats_10.*")["suspiciousness"] == 1.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_10.*").suspiciousness == 1.0
        assert self.get_metric() == metric_cnt + 1
        assert self.get_metric_many_requests() == many_requests_cnt
        assert self.get_metric_many_requests_mobile() == many_requests_mobile_cnt

        # disable many requests in its
        # suspiciousness request - 200
        with open(self.many_requests_file, "w") as f:
            f.write("")
        time.sleep(WATCHER_PERIOD + 0.1)

        many_requests_cnt = self.get_metric_many_requests()
        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "cats_6", spravka))
        assert resp.getcode() == 200
        assert self.unified_agent.wait_log_line_with_query(r".*cats_6.*")["suspiciousness"] == 1.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_6.*").suspiciousness == 1.0
        assert self.get_metric() == metric_cnt + 1
        assert self.get_metric_many_requests() == many_requests_cnt

    def test_suspiciousness_ban(self):
        ip = '22.13.42.8'
        spravka = GetSpravkaForAddr(self, ip)

        self.ban_spravka()

        self.cbb.add_text_block(
            CBB_FLAG_SUSPICIOUSNESS, "cgi=/.*suspected_robot.*/"
        )
        time.sleep(CBB_SYNC_PERIOD + 0.1)

        self.unified_agent.pop_daemon_logs()

        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "cats_1", spravka))
        assert resp.headers[SUSPICIOUSNESS_HEADER] == "1.0"
        assert self.unified_agent.wait_log_line_with_query(r".*cats_1.*")["suspiciousness"] == 1.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_1.*").suspiciousness == 1.0
        assert self.get_metric() == metric_cnt + 1

        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "i_am_suspected_robot", None))
        assert not IsCaptchaRedirect(resp)
        assert resp.headers[SUSPICIOUSNESS_HEADER] == "1.0"
        assert self.unified_agent.wait_log_line_with_query(r".*i_am_suspected_robot.*")["suspiciousness"] == 1.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*i_am_suspected_robot.*").suspiciousness == 1.0
        assert self.get_metric() == metric_cnt + 1

        with open(self.ban_file, "w") as f:
            f.write("web")
        time.sleep(WATCHER_PERIOD + 0.1)

        # suspiciousness ban not work with spravka
        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "cats_2", spravka))
        assert not IsCaptchaRedirect(resp)
        assert resp.headers[SUSPICIOUSNESS_HEADER] == "1.0"
        log_line = self.unified_agent.wait_log_line_with_query(r".*cats_2.*")
        assert log_line["verdict"] == 'NEUTRAL'
        assert log_line["suspiciousness"] == 1.0
        assert self.get_metric() == metric_cnt + 1

        # suspiciousness ban work with cbb
        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "i_am_suspected_robot", None))
        assert IsCaptchaRedirect(resp)
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        log_line = self.unified_agent.wait_log_line_with_query(r".*i_am_suspected_robot.*")
        assert log_line["suspiciousness"] == 1.0
        assert log_line['verdict'] == 'ENEMY'

        cacher_log_line = self.unified_agent.wait_cacher_log_line_with_query(r".*i_am_suspected_robot.*")
        assert cacher_log_line.suspiciousness == 1.0
        assert cacher_log_line.verdict == 'ENEMY'

        assert self.get_metric() == metric_cnt + 1

        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "cats_3", "invalid_spravka"))
        assert not IsCaptchaRedirect(resp)
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert self.unified_agent.wait_log_line_with_query(r".*cats_3.*")["suspiciousness"] == 0.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_3.*").suspiciousness == 0.0
        assert self.get_metric() == metric_cnt

    def test_suspiciousness_block(self):
        ip = '22.13.42.9'
        spravka = GetSpravkaForAddr(self, ip)

        self.ban_spravka()

        self.unified_agent.pop_daemon_logs()
        time.sleep(0.5)

        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "cats_1", spravka))
        assert resp.headers[SUSPICIOUSNESS_HEADER] == "1.0"
        assert self.unified_agent.wait_log_line_with_query(r".*cats_1.*")["suspiciousness"] == 1.0
        assert self.unified_agent.wait_cacher_log_line_with_query(r".*cats_1.*").suspiciousness == 1.0
        assert self.get_metric() == metric_cnt + 1

        with open(self.block_file, "w") as f:
            f.write("web")
        time.sleep(WATCHER_PERIOD + 0.1)

        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "cats_2", spravka))
        assert IsBlockedResponse(resp)
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        log_line = self.unified_agent.wait_log_line_with_query(r".*cats_2.*")
        assert log_line['cacher_blocked']
        assert log_line['cacher_block_reason'] == 'SUSPICIOUS'
        assert log_line["suspiciousness"] == 1.0

        cacher_log_line = self.unified_agent.wait_cacher_log_line_with_query(r".*cats_2.*")
        assert cacher_log_line.cacher_blocked
        assert cacher_log_line.cacher_block_reason == 'SUSPICIOUS'
        assert cacher_log_line.suspiciousness == 1.0

        assert self.get_metric() == metric_cnt + 1

        metric_cnt = self.get_metric()
        resp = self.send_request(self.get_search_req(ip, "cats_3", "invalid_spravka"))
        assert not IsBlockedResponse(resp)
        assert SUSPICIOUSNESS_HEADER not in resp.headers
        assert self.unified_agent.wait_log_line_with_query(r".*cats_3.*")["suspiciousness"] == 0.0
        assert self.get_metric() == metric_cnt
