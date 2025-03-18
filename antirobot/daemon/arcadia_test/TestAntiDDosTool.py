import random
import time

import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    VALID_L_COOKIE,
    VALID_I_COOKIE,
    VALID_FUID,
    asserts,
    Fullreq,
    IsCaptchaRedirect,
)

CBB_SYNC_PERIOD = 1
LONG_TEST_TIMEOUT = 30
IP_BLOCK_RANGE_BEGIN = "1.2.3.4"
IP_BLOCK_RANGE_END = "1.2.3.10"
IP_TO_BLOCK = "1.2.3.5"
IPV6_TO_BLOCK = "2a02:6b8:0:40c:7ba0:f412:fe23:bf71"
CBB_FLAG_MANUAL = 10  # manual blocks by ip ranges
CBB_FLAG_MANUAL_RE = 11
CBB_FLAG_BAN_IP_BASED = 12
CBB_FLAG_BAN_FARMABLE = 13


def get_spravka_based_request(spravka):
    return Fullreq(
        "http://yandex.ru/search?text=test_req&param1=buble1-goom",
        headers={
            "X-Forwarded-For-Y": IP_TO_BLOCK,
            "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
            "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
            "Host": "bad-host.yandex.ru:15879",
            "Referer": "http://www.yandex.ru/bad-referer",
            "Cookie": "spravka=" + spravka,
        },
    )


NonBlockedRequest = Fullreq(
    "http://yandex.ru/search/yandsearch?text=test_req&param1=buble-goom",
    headers={
        "X-Forwarded-For-Y": "1.2.3.1",
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) testo/2.10.289 Version/12.02",
        "Host": "host.yandex.ru:15879",
        "Referer": "http://www.yandex.ru/referer",
    },
)

BlockedRequest = Fullreq(
    "http://yandex.ru/search?text=test_req&param1=buble1-goom",
    headers={
        "X-Forwarded-For-Y": IP_TO_BLOCK,
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
        "Host": "bad-host.yandex.ru:15879",
        "Referer": "http://www.yandex.ru/bad-referer",
    },
)

IpBasedRequest = Fullreq(
    "http://yandex.ru/search?text=test_req&param1=buble1-goom",
    headers={
        "X-Forwarded-For-Y": IP_TO_BLOCK,
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
        "Host": "bad-host.yandex.ru:15879",
        "Referer": "http://www.yandex.ru/bad-referer",
    },
)

IpV6BasedRequest = Fullreq(
    "http://yandex.ru/search?text=test_req&param1=buble1-goom",
    headers={
        "X-Forwarded-For-Y": IPV6_TO_BLOCK,
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
        "Host": "bad-host.yandex.ru:15879",
        "Referer": "http://www.yandex.ru/bad-referer",
    },
)


LoginBasedRequest = Fullreq(
    "http://yandex.ru/search?text=test_req&param1=buble1-goom",
    headers={
        "X-Forwarded-For-Y": IP_TO_BLOCK,
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
        "Host": "bad-host.yandex.ru:15879",
        "Referer": "http://www.yandex.ru/bad-referer",
        "Cookie": ("L=%s" % VALID_L_COOKIE),  # newly registered test user
    },
)


ICookieBasedRequest = Fullreq(
    "http://yandex.ru/search?text=test_req&param1=buble1-goom",
    headers={
        "X-Forwarded-For-Y": IP_TO_BLOCK,
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
        "Host": "bad-host.yandex.ru:15879",
        "Referer": "http://www.yandex.ru/bad-referer",
        "X-Yandex-ICookie": VALID_I_COOKIE,
    },
)

FuidBasedRequest = Fullreq(
    "http://yandex.ru/search?text=test_req&param1=buble1-goom",
    headers={
        "X-Forwarded-For-Y": IP_TO_BLOCK,
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
        "Host": "bad-host.yandex.ru:15879",
        "Referer": "http://www.yandex.ru/bad-referer",
        "Cookie": ("fuid01=%s" % VALID_FUID),
    },
)

FuidBasedDiskRequest = Fullreq(
    "https://disk.yandex.ru/notes/?from=yabrowser",
    headers={
        "X-Forwarded-For-Y": "1.2.3.1",
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) testo/2.10.289 Version/12.02",
        "Host": "disk.yandex.ru",
        "Referer": "http://www.yandex.ru/referer",
        "Cookie": ("fuid01=%s" % VALID_FUID),
    },
)


class TestAntiDDosTool(AntirobotTestSuite):
    options = {
        "DisableBansByFactors": 1,
        "DDosFlag1BlockEnabled": 0,
        "DDosFlag2BlockEnabled": 0,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbApiTimeout": "0.3s",
        "cbb_ip_flag": CBB_FLAG_MANUAL,
        "cbb_re_flag": CBB_FLAG_MANUAL_RE,
        "CbbFlagIpBasedIdentificationsBan": CBB_FLAG_BAN_IP_BASED,
        "cbb_farmable_ban_flag": CBB_FLAG_BAN_FARMABLE,
    }

    BLOCK_RULES = (
        r'header["host"]=/bad-host\..+/',
        r'header["User-Agent"]=/.+Testo.+/',
        r"ip=1.2.3.5",
        r"ip=1.2.3.4/30",
        r"doc=/.*\/search/",
        r"cgi=/.*param1=buble1.*/",
        r'header["host"]!=/host\..+/',
        r'header["User-Agent"]!=/.+testo.+/',
        r"doc!=/.*\/yandsearch/",
        r"cgi!=/.*param1=buble-.*/",
    )

    TEST_RULES = (
        (r'header["host"]=/some-host\..+/', 1),
        (r'header["host"]=/some-host\..=/', 1),
        (r'header["host"]=/some-host\..)/', 0),
        (r'header["host"]=/some-host\..{/', 0),
        (r'header["host"]=/some-host\..*/', 1),
        (r'header["host"]=/some-host\..\/', 0),
        (r'header["host"]=/some-host\..#/', 1),
        (r"ip_from=1", 1),
    )

    @classmethod
    def setup_subclass(cls):
        spravka_domain = "yandex.ru"
        valid_spravka = cls.send_request(
            f"/admin?action=getspravka&domain={spravka_domain}",
            cls.antirobot.admin_port,
        ).read().decode().strip()

        asserts.AssertSpravkaValid(cls, valid_spravka, domain=spravka_domain, key=cls.spravka_data_key())
        cls.SpravkaBasedRequest = get_spravka_based_request(valid_spravka)

    def append_rule(self, rule, rule_id=None):
        self.cbb.add_text_block(CBB_FLAG_MANUAL_RE, rule, rule_id=rule_id)
        time.sleep(CBB_SYNC_PERIOD + 1)

    def remove_rule(self, rule):
        self.cbb.remove_text_block(CBB_FLAG_MANUAL_RE, rule)
        time.sleep(CBB_SYNC_PERIOD + 1)

    def test_does_not_block(self):
        asserts.AssertNotBlocked(self.send_request(BlockedRequest))
        asserts.AssertNotBlocked(self.send_request(NonBlockedRequest))

    def test_nonblock_rule_is_higher(self):
        self.append_rule(r'header["host"]=/bad-host\..+/')
        asserts.AssertEventuallyBlocked(self, BlockedRequest)

        self.append_rule(r'nonblock=yes; header["User-Agent"]=/.+Testo.+/')
        asserts.AssertEventuallyNotBlocked(self, BlockedRequest)

    def test_nonblock_rule_is_higher_than_manual(self):
        self.cbb.add_block(CBB_FLAG_MANUAL, IP_TO_BLOCK, IP_TO_BLOCK, None)
        time.sleep(CBB_SYNC_PERIOD + 1)
        asserts.AssertEventuallyBlocked(self, BlockedRequest)

        self.append_rule(r"nonblock=yes; doc=/\/search.*/")
        asserts.AssertEventuallyNotBlocked(self, BlockedRequest)

    @pytest.mark.parametrize("rule", BLOCK_RULES)
    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_blocks(self, rule):
        self.append_rule(rule)
        resp = self.send_request(NonBlockedRequest)
        asserts.AssertNotBlocked(resp)

        asserts.AssertBlocked(self.send_request(BlockedRequest))

        # now remove the rule from cbb
        self.remove_rule(rule)
        asserts.AssertNotBlocked(self.send_request(BlockedRequest))

    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_rule_with_external_cbb_group(self):
        externalIpGroup = 1

        self.cbb.add_block(
            externalIpGroup, IP_BLOCK_RANGE_BEGIN, IP_BLOCK_RANGE_END, None
        )

        self.append_rule("ip_from={}".format(externalIpGroup))
        time.sleep(CBB_SYNC_PERIOD + 1)
        resp = self.send_request(NonBlockedRequest)
        asserts.AssertNotBlocked(resp)

        asserts.AssertEventuallyBlocked(self, BlockedRequest)

        # now remove the address from cbb group
        self.cbb.remove_block(externalIpGroup, IP_BLOCK_RANGE_BEGIN)
        time.sleep(CBB_SYNC_PERIOD + 1)

        asserts.AssertEventuallyNotBlocked(self, BlockedRequest)

    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_empty_rule_doesnt_block(self):
        asserts.AssertNotBlocked(self.send_request(NonBlockedRequest))
        self.append_rule('rem="test"')
        asserts.AssertNotBlocked(self.send_request(NonBlockedRequest))

    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_unblock_after_add_to_nonblocking_list(self):
        externalIpGroup = 1

        self.cbb.add_block(
            externalIpGroup, IP_BLOCK_RANGE_BEGIN, IP_BLOCK_RANGE_END, None
        )

        self.append_rule("ip_from=%d" % externalIpGroup)
        time.sleep(CBB_SYNC_PERIOD + 1)
        resp = self.send_request(NonBlockedRequest)
        asserts.AssertNotBlocked(resp)

        asserts.AssertEventuallyBlocked(self, BlockedRequest)

        # now add the address to the nonblocking group
        self.cbb.add_block(
            int(self.antirobot.dump_cfg()["CbbFlagNonblocking"]),
            IP_TO_BLOCK,
            IP_TO_BLOCK,
            None,
        )
        time.sleep(CBB_SYNC_PERIOD + 1)
        asserts.AssertEventuallyNotBlocked(self, BlockedRequest)

    # @pytest.mark.parametrize("rule, expectedLength", TEST_RULES)
    # def test_check_cbb_rules_correctness(self, rule, expectedLength):
    #     def getRulesFromAntirobot():
    #         resp = self.send_request(
    #             r"/admin?action=get_cbb_rules&service=other"
    #         )
    #         return [r for r in resp.read().decode().split("\n") if len(r) > 0]

    #     self.append_rule(rule)
    #     asserts.AssertEventuallyTrue(
    #         lambda: len(getRulesFromAntirobot()) == expectedLength,
    #         secondsBetweenCalls=0.5,
    #     )

    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_check_cbb_rules_logging(self):
        rule_id = random.randrange(2**32)

        def getCbbLogLinesFromAntirobotEventLog():
            events = self.unified_agent.pop_event_logs(["TCbbRulesUpdated"])

            result = []

            for event in events:
                if len(event.Event.ParseResults) != 0:
                    parse_result = event.Event.ParseResults[0]
                    if parse_result.Rule == f"ip_from=1; rule_id={rule_id}" and parse_result.Status == "OK":
                        result.append(event)

            return result

        externalIpGroup = 1
        self.append_rule("ip_from={}".format(externalIpGroup), rule_id=rule_id)

        asserts.AssertEventuallyTrue(
            lambda: len(getCbbLogLinesFromAntirobotEventLog()) > 0,
            secondsBetweenCalls=0.5,
        )

    def test_block_xml_search(self):
        externalIpGroup = 1
        self.cbb.add_block(
            externalIpGroup, IP_BLOCK_RANGE_BEGIN, IP_BLOCK_RANGE_END, None
        )
        self.append_rule("ip_from={}".format(externalIpGroup))
        time.sleep(CBB_SYNC_PERIOD + 1)

        blockedRequest = Fullreq(
            "http://xmlsearch.yandex.ru/xmlsearch?text=123",
            headers={"X-Forwarded-For-Y": IP_TO_BLOCK},
        )
        asserts.AssertEventuallyBlocked(self, blockedRequest)

        resp = self.send_request(
            Fullreq(
                "http://xmlsearch.yandex.ru/xmlsearch?text=123",
                headers={"X-Forwarded-For-Y": "1.2.3.1"},
            )
        )
        asserts.AssertNotBlocked(resp)

    def test_ban_ip_based(self):
        self.cbb.add_block(
            CBB_FLAG_BAN_IP_BASED, IP_TO_BLOCK, IP_TO_BLOCK, None
        )
        self.cbb.add_block(
            CBB_FLAG_BAN_IP_BASED, IPV6_TO_BLOCK, IPV6_TO_BLOCK, None
        )
        time.sleep(CBB_SYNC_PERIOD + 1)

        asserts.AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(self.send_request(IpBasedRequest))
        )
        asserts.AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(self.send_request(IpV6BasedRequest))
        )

        assert not IsCaptchaRedirect(self.send_request(LoginBasedRequest))
        assert not IsCaptchaRedirect(self.send_request(self.SpravkaBasedRequest))
        assert not IsCaptchaRedirect(self.send_request(ICookieBasedRequest))
        assert IsCaptchaRedirect(self.send_request(FuidBasedRequest))
        assert not IsCaptchaRedirect(self.send_request(FuidBasedDiskRequest))

    def test_ban_farmable(self):
        self.cbb.add_block(
            CBB_FLAG_BAN_FARMABLE, IP_TO_BLOCK, IP_TO_BLOCK, None
        )
        self.cbb.add_block(
            CBB_FLAG_BAN_FARMABLE, IPV6_TO_BLOCK, IPV6_TO_BLOCK, None
        )
        time.sleep(CBB_SYNC_PERIOD + 1)

        asserts.AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(self.send_request(IpBasedRequest))
        )
        asserts.AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(self.send_request(IpV6BasedRequest))
        )
        asserts.AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(self.send_request(ICookieBasedRequest))
        )
        asserts.AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(self.send_request(FuidBasedRequest))
        )

        assert not IsCaptchaRedirect(self.send_request(LoginBasedRequest))
        assert not IsCaptchaRedirect(self.send_request(self.SpravkaBasedRequest))

    def test_dont_ban_cannot_show_captcha(self):
        not_banable_request = Fullreq(
            "http://yandex.ru/do_not_ban_pls",
            headers={
                "X-Forwarded-For-Y": IP_TO_BLOCK,
                "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
                "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
                "Host": "bad-host.yandex.ru:15879",
                "Referer": "http://www.yandex.ru/bad-referer",
            },
        )
        self.cbb.add_block(
            CBB_FLAG_BAN_FARMABLE, IP_TO_BLOCK, IP_TO_BLOCK, None
        )
        self.cbb.add_block(
            CBB_FLAG_BAN_FARMABLE, IPV6_TO_BLOCK, IPV6_TO_BLOCK, None
        )
        time.sleep(CBB_SYNC_PERIOD + 1)

        # To ensure synchronization
        asserts.AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(self.send_request(IpBasedRequest))
        )

        assert not IsCaptchaRedirect(self.send_request(not_banable_request))
