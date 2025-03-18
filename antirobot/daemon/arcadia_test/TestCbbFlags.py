import hashlib
import itertools
from pathlib import Path
import time

import pytest

from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    IsCaptchaRedirect,
    IsBlockedResponse,
    GenRandomIP,
    ICOOKIE_HEADER,
    IP_HEADER,
    VALID_I_COOKIE,
    VALID_I_COOKIE_TIMESTAMP,
)
from antirobot.daemon.arcadia_test.util.asserts import (
    AssertEventuallyTrue,
    AssertEventuallyBlocked,
    AssertEventuallyNotBlocked,
    AssertBlocked,
)
from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite, TEST_DATA_ROOT
from antirobot.daemon.arcadia_test.util.captcha_page import HtmlCheckboxCaptchaPage
from antirobot.daemon.arcadia_test.util.spravka import GetSpravkaForAddr

CBB_SYNC_PERIOD = 1
CBB_FLAG_CAPTCHA_BY_REGEXP = 13
CBB_FLAG_NONBLOCKING = 14
CBB_FLAG_BAN_IP_BASED = 225
CBB_FLAG_IGNORE_SPRAVKA = 328
CBB_FLAG_CUT_REQUESTS = 423
CBB_FLAG_NOT_BANNED = 499
CBB_FLAG_RE_LIST = 3500
CBB_FLAG_CHECKBOX_BLACKLIST = 501
CBB_FLAG_CAN_SHOW_CAPTCHA = 502
MIN_REQUESTS_WITH_SPRAVKA = 20
CBB_CACHE_PERIOD = 1
CBB_FLAG_MANUAL_RE = 11
CBB_FLAG_BAN_FW_RE = 974
CBB_FLAG_BAN_FW_IP = 824
CBB_ADD_BAN_FW_PERIOD = 2

MAYBAN_HEADER = "X-Antirobot-MayBanFor-Y"
START_TIME_HEADER = "X-Start-Time"
FORCE_HOST_HEADER = "X-Antirobot-Service-Y"

REGULAR_SEARCH = "http://yandex.ru/search?text=cats"

NonBlockedRequest = Fullreq(
    "http://yandex.ru/search/yandsearch?text=test_req&param1=buble-goom",
    headers={
        "X-req-id": "1346776354215268-879007856167401084457064-6-025",
        "X-forwarded-for-y": "1.2.3.1",
        "Referer": "http://www.yandex.ru/referer",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) testo/2.10.289 Version/12.02",
        "Host": "host.yandex.ru:15879",
    },
)

BlockedRequest = Fullreq(
    "http://yandex.ru/search/yandsearch?text=test_req&param1=buble-goom",
    headers={
        "X-forwarded-for-y": "1.2.3.1",
        "X-req-id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) testo/2.10.289 Version/12.02",
        "Host": "host.yandex.ru:15879",
        "Referer": "http://www.yandex.ru/referer",
    },
)


class TestCbbFlags(AntirobotTestSuite):
    options = {
        "bans_enabled": True,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbApiTimeout": "0.3s",
        "cbb_captcha_re_flag": CBB_FLAG_CAPTCHA_BY_REGEXP,
        "CbbFlagIpBasedIdentificationsBan": CBB_FLAG_BAN_IP_BASED,
        "CbbFlagNonblocking": CBB_FLAG_NONBLOCKING,
        "CbbFlagNotBanned": CBB_FLAG_NOT_BANNED,
        "cbb_checkbox_blacklist_flag": CBB_FLAG_CHECKBOX_BLACKLIST,
        "cbb_can_show_captcha_flag": CBB_FLAG_CAN_SHOW_CAPTCHA,
        "MinRequestsWithSpravka": MIN_REQUESTS_WITH_SPRAVKA,
        "CbbFlagBanFWSourceIp": CBB_FLAG_BAN_FW_RE,
        "CbbFlagBanFWSourceIpsContainer": CBB_FLAG_BAN_FW_IP,
        "CbbAddBanFWPeriod": CBB_ADD_BAN_FW_PERIOD,
        "inherit_bans@img": ["web"],
    }

    fury_args = ["--button-check", "success"]

    def teardown_method(self, method):
        self.cbb.clear([CBB_FLAG_CAPTCHA_BY_REGEXP])

    def test_show_captcha_by_regexp(self):
        humanRequest = Fullreq(
            "http://yandex.ru/search?text=i_am_human",
            headers={IP_HEADER: GenRandomIP()},
        )
        robotRequest = Fullreq(
            "http://yandex.ru/search?text=i_am_robot",
            headers={IP_HEADER: GenRandomIP()},
        )

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*robot.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.antirobot.send_request(robotRequest))
        assert not IsCaptchaRedirect(self.antirobot.send_request(humanRequest))

    def test_dont_show_regexp_captcha_to_not_may_ban(self):
        mayBanForRequest = Fullreq(
            "http://yandex.ru/search?text=i_am_web_robot",
            headers={IP_HEADER: GenRandomIP()},
        )
        mayNotBanForRequest = Fullreq(
            "http://market.yandex.ru/api/clickproxy?text=i_am_market_robot",
            headers={IP_HEADER: GenRandomIP()},
        )
        mayNotBanForRequestButHeaderAllows = Fullreq(
            "https://eda.yandex/restaurant/teremok_novogireevo?text=i_am_eda_robot",
            headers={IP_HEADER: GenRandomIP(), MAYBAN_HEADER: 1},
        )
        mayNotBanForRequestNoHeader = Fullreq(
            "https://eda.yandex/restaurant/teremok_novogireevo?text=i_am_eda_robot",
            headers={IP_HEADER: GenRandomIP()},
        )

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*robot.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.antirobot.send_request(mayBanForRequest))
        assert not IsCaptchaRedirect(
            self.antirobot.send_request(mayNotBanForRequest)
        )
        assert IsCaptchaRedirect(
            self.antirobot.send_request(mayNotBanForRequestButHeaderAllows)
        )
        assert not IsCaptchaRedirect(
            self.antirobot.send_request(mayNotBanForRequestNoHeader)
        )

    def test_whitelisted_in_blocks_can_be_banned_by_threshold(self):
        ip = GenRandomIP()

        self.cbb.add_block(CBB_FLAG_NONBLOCKING, ip, ip, None)
        time.sleep(CBB_SYNC_PERIOD + 1)

        AssertEventuallyTrue(lambda: IsCaptchaRedirect(self.send_fullreq(
            REGULAR_SEARCH,
            headers={IP_HEADER: ip},
        )))

    def test_re_list(self):
        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, f"cgi=#{CBB_FLAG_RE_LIST}")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert not IsCaptchaRedirect(self.send_fullreq(
            "http://yandex.ru/search?text=i_am_robot",
            headers={IP_HEADER: GenRandomIP()},
        ))

        self.cbb.add_re_block(CBB_FLAG_RE_LIST, "/.*robot.*/\n/.*replicant.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.send_fullreq(
            "http://yandex.ru/search?text=i_am_robot",
            headers={IP_HEADER: GenRandomIP()},
        ))

        assert IsCaptchaRedirect(self.send_fullreq(
            "http://yandex.ru/search?text=i_am_replicant",
            headers={IP_HEADER: GenRandomIP()},
        ))

    def test_inherit_bans(self):
        ip = GenRandomIP()

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, f"service_type=/web/;ip={ip}")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.send_fullreq(
            "https://yandex.ru/search?text=hello",
            headers={
                IP_HEADER: ip,
                ICOOKIE_HEADER: VALID_I_COOKIE,
            },
        ))
        time.sleep(1)

        assert IsCaptchaRedirect(self.send_fullreq(
            "https://yandex.ru/images/search?text=test",
            headers={
                IP_HEADER: ip,
                ICOOKIE_HEADER: VALID_I_COOKIE,
            },
        ))

    def test_checkbox_blacklist(self):
        ip = GenRandomIP()

        self.cbb.add_text_block(CBB_FLAG_CHECKBOX_BLACKLIST, f"ip={ip}")
        time.sleep(CBB_SYNC_PERIOD + 1)

        headers = {IP_HEADER: ip}
        page = HtmlCheckboxCaptchaPage(self, "yandex.ru", headers=headers)
        resp = self.send_fullreq(page.get_check_captcha_url(), headers=headers)
        assert IsCaptchaRedirect(resp)

    @pytest.mark.parametrize('url', [
        "https://yandex.com/blogs?id=1&cat=theme",  # may_ban=True
        "https://yandex.com/dddddddd",  # may_ban=False
    ])
    def test_can_show_captcha_flag(self, url):
        ip = GenRandomIP()

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, f"ip={ip}")
        time.sleep(CBB_SYNC_PERIOD + 1)

        req = Fullreq(
            url,
            headers={IP_HEADER: ip},
        )

        assert not IsCaptchaRedirect(self.send_request(req))

        self.cbb.add_text_block(CBB_FLAG_CAN_SHOW_CAPTCHA, f"ip={ip}")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.send_request(req))

    @pytest.mark.parametrize("valid", [True, False])
    def test_valid_autoru_tamper(self, valid):
        ip = GenRandomIP()

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "valid_autoru_tamper=no")
        time.sleep(CBB_SYNC_PERIOD + 1)

        if valid:
            with open(TEST_DATA_ROOT / "data" / "autoru_tamper_salt") as tamper_salt_file:
                tamper_salt = tamper_salt_file.read().strip()
        else:
            tamper_salt = "abacaba"

        device_uid = "some_device"
        tamper = hashlib.md5(("text=test" + device_uid + tamper_salt + "0").encode()).hexdigest()

        assert IsCaptchaRedirect(self.send_fullreq(
            "https://realty.yandex.ru/search?text=test",
            headers={
                "X-Antirobot-MayBanFor-Y": 1,
                "X-Antirobot-Service-Y": "realty",
                "X-Device-UID": device_uid,
                "X-Timestamp": tamper,
                IP_HEADER: ip,
            },
        )) != valid

    def test_regex_on_cookie_age(self):
        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cookie_age>36000")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert not IsCaptchaRedirect(self.send_fullreq(
            "https://yandex.ru/search?text=i_am_robot",
            headers={
                IP_HEADER: GenRandomIP(),
                ICOOKIE_HEADER: VALID_I_COOKIE,
                START_TIME_HEADER: int(VALID_I_COOKIE_TIMESTAMP + 9*3600),
            },
        ))

        assert not IsCaptchaRedirect(self.send_fullreq(
            "https://yandex.ru/search?text=i_am_robot",
            headers={
                IP_HEADER: GenRandomIP(),
            },
        ))

        assert IsCaptchaRedirect(self.send_fullreq(
            "https://yandex.ru/search?text=i_am_robot",
            headers={
                IP_HEADER: GenRandomIP(),
                ICOOKIE_HEADER: VALID_I_COOKIE,
                START_TIME_HEADER: int(VALID_I_COOKIE_TIMESTAMP + 11*3600),
            },
        ))

        assert IsCaptchaRedirect(self.send_fullreq(
            "https://yandex.ru/search?text=i_am_robot",
            headers={
                IP_HEADER: GenRandomIP(),
                "Cookie": f"yandexuid=1234{VALID_I_COOKIE_TIMESTAMP};",
                START_TIME_HEADER: int(VALID_I_COOKIE_TIMESTAMP + 11*3600),
            },
        ))

    def test_ban_ip_fw(self):
        ip = GenRandomIP()

        self.cbb.add_text_block(CBB_FLAG_BAN_FW_RE, f"ip={ip}")
        time.sleep(CBB_SYNC_PERIOD + 1)

        self.send_fullreq(
            "https://yandex.ru/search?text=i_am_robot",
            headers={
                IP_HEADER: ip,
            })

        time.sleep(CBB_ADD_BAN_FW_PERIOD + 1)
        assert self.get_last_event_in_daemon_logs().get('ban_fw_source_ip')
        assert self.unified_agent.get_last_cacher_log().ban_fw_source_ip is True

        resp = self.cbb.fetch_flag_data(CBB_FLAG_BAN_FW_IP, with_format=['range_src'])

        assert resp.readlines() == [(ip + '\n').encode(), b'\n']

    def test_factor_conditions(self):
        ip = GenRandomIP()

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "factor['AcceptLanguageHasRussian']==1")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.send_fullreq(
            "https://yandex.ru/search?text=i_am_robot",
            headers={
                IP_HEADER: ip,
                "Accept-Language": "RU",
            },
        ))


class TestCbbFlagsRobotset(AntirobotTestSuite):
    options = {
        "bans_enabled": True,
        "DisableBansByFactors": 1,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbApiTimeout": "0.3s",
        "cbb_captcha_re_flag": CBB_FLAG_CAPTCHA_BY_REGEXP,
        "CbbFlagIpBasedIdentificationsBan": CBB_FLAG_BAN_IP_BASED,
        "CbbFlagNonblocking": CBB_FLAG_NONBLOCKING,
        "CbbFlagNotBanned": CBB_FLAG_NOT_BANNED,
        "MinRequestsWithSpravka": MIN_REQUESTS_WITH_SPRAVKA,
        "inherit_bans@img": ["web"],
    }

    def test_dont_add_robotset_by_regexp(self):
        ip = GenRandomIP()
        robotRequest = Fullreq(
            "http://yandex.ru/search?text=i_am_robot",
            headers={IP_HEADER: ip},
        )
        # human on same ip
        humanRequest = Fullreq(
            "http://yandex.ru/search?text=i_am_human",
            headers={IP_HEADER: ip},
        )

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*robot.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.antirobot.send_request(robotRequest))
        assert not IsCaptchaRedirect(self.antirobot.send_request(humanRequest))
        # check don't add in robotset in processor
        self.unified_agent.wait_log_line_with_query(".*i_am_robot.*")
        assert not IsCaptchaRedirect(self.antirobot.send_request(humanRequest))


class TestCbbWhitelistFlag(AntirobotTestSuite):
    options = {
        "bans_enabled": True,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbApiTimeout": "0.3s",
        "cbb_captcha_re_flag": CBB_FLAG_CAPTCHA_BY_REGEXP,
        "CbbFlagIpBasedIdentificationsBan": CBB_FLAG_BAN_IP_BASED,
        "CbbFlagNonblocking": CBB_FLAG_NONBLOCKING,
        "CbbFlagNotBanned": CBB_FLAG_NOT_BANNED,
        "MinRequestsWithSpravka": MIN_REQUESTS_WITH_SPRAVKA,
    }

    def test_dont_show_captcha_by_regexp(self):
        robotIp = GenRandomIP()
        robotRequest = Fullreq(
            "http://yandex.ru/search?text=i_am_robot",
            headers={IP_HEADER: robotIp},
        )

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*robot.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.antirobot.send_request(robotRequest))
        assert not self.get_last_event_in_daemon_logs().get('cbb_whitelist')

        self.cbb.add_text_block(CBB_FLAG_NOT_BANNED, "header['" + IP_HEADER + "']=/" + robotIp + "/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert not IsCaptchaRedirect(self.antirobot.send_request(robotRequest))
        assert self.get_last_event_in_daemon_logs().get('cbb_whitelist')

    def test_update_multiple_flags(self):
        ip = GenRandomIP()

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*robot.*/")
        self.cbb.add_text_block(CBB_FLAG_NOT_BANNED, f"header['{IP_HEADER}']=/{ip}/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert not IsCaptchaRedirect(self.antirobot.ping_search(ip, query="i_am_robot"))
        assert self.get_last_event_in_daemon_logs().get('cbb_whitelist')


class TestCbbFlagsIgnoreSpravka(AntirobotTestSuite):
    options = {
        "bans_enabled": True,
        "threshold": -10,
        "CbbSyncPeriod": "1s",
        "CbbApiTimeout": "0.3s",
        "CbbFlagIgnoreSpravka": CBB_FLAG_IGNORE_SPRAVKA,
        "SpravkaPenalty": 0,
        "MinRequestsWithSpravka": MIN_REQUESTS_WITH_SPRAVKA,
    }

    def test_ignore_spravka(self):
        self.cbb.add_text_block(CBB_FLAG_IGNORE_SPRAVKA, "cgi=/.*ignore.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        ignoreText = "http://yandex.ru/search?text=ignore"
        immuneText = "http://yandex.ru/search?text=immune"

        requestWithoutImmunity = self.get_spravka_based_request(ignoreText, GenRandomIP())
        requestWithImmunity = self.get_spravka_based_request(immuneText, GenRandomIP())

        assert (
            self.requests_count_before_ban(self.antirobot, requestWithoutImmunity)
            < MIN_REQUESTS_WITH_SPRAVKA
        )

        assert (
            self.requests_count_before_ban(self.antirobot, requestWithImmunity)
            >= MIN_REQUESTS_WITH_SPRAVKA
        )

    def get_spravka_based_request(self, search_text, ip):
        return Fullreq(
            search_text,
            headers={
                IP_HEADER: ip,
                "Cookie": "spravka=" + self.get_valid_spravka(),
            },
        )

    def requests_count_before_ban(self, daemon, request):
        for i in itertools.count():
            if IsCaptchaRedirect(daemon.send_request(request)):
                return i + 1

            time.sleep(0.1)


class TestCbbFlagsCutRequests(AntirobotTestSuite):
    options = {
        "bans_enabled": True,
        "threshold": -10,
        "CbbSyncPeriod": "1s",
        "CbbApiTimeout": "0.3s",
        "CbbFlagCutRequests": CBB_FLAG_CUT_REQUESTS,
        "MinRequestsWithSpravka": MIN_REQUESTS_WITH_SPRAVKA,
    }

    def test_cut_requests(self):
        self.cbb.add_text_block(CBB_FLAG_CUT_REQUESTS, "header['X-Start-Time']=/.*(0|4|8)/;service_type=/web/")
        self.cbb.add_text_block(CBB_FLAG_CUT_REQUESTS, "arrival_time=/.*(2|6)/;service_type=/web/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        for sec in range(10):
            request = Fullreq(REGULAR_SEARCH + str(sec), headers={IP_HEADER: GenRandomIP(), START_TIME_HEADER: "137994789316704" + str(sec)})
            self.send_request(request)
        res = self.unified_agent.wait_log_line_with_query(r".*cats.*", True)
        assert all(r["req_url"][-1] not in "02468" for r in res)
        assert all(r["req_url"][-1] in "13579" for r in res)


class TestCbbFlagsHighThreshold(AntirobotTestSuite):
    DataDir = Path.cwd()
    whitelist = DataDir / "whitelist_ips"
    whitelist_all = DataDir / "whitelist_ips_all"
    img_whitelist = DataDir / "whitelist_ips_img"
    market_whitelist1 = DataDir / "whitelist_ips_market1"
    market_whitelist2 = DataDir / "whitelist_ips_market2"

    options = {
        "WhiteList": whitelist.name,
        "WhitelistsDir": DataDir,
        "bans_enabled": True,
        "DDosFlag1BlockEnabled": 0,
        "DDosFlag2BlockEnabled": 0,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbApiTimeout": "0.3s",
        "CbbFlagIpBasedIdentificationsBan": CBB_FLAG_BAN_IP_BASED,
        "cbb_captcha_re_flag": CBB_FLAG_CAPTCHA_BY_REGEXP,
        "CbbFlagNonblocking": CBB_FLAG_NONBLOCKING,
        "DisableBansByFactors": 1,
        "MinRequestsWithSpravka": MIN_REQUESTS_WITH_SPRAVKA,
        "SpravkaPenalty": 0,
        "whitelists@market": [str(market_whitelist1.name), str(market_whitelist2.name)],
        "cacher_threshold@web": -1000000
    }

    captcha_args = ['--generate', 'correct', '--check', 'success']

    @classmethod
    def setup_class(cls):
        for f in [
            cls.whitelist,
            cls.whitelist_all,
            cls.img_whitelist,
            cls.market_whitelist1,
            cls.market_whitelist2,
        ]:
            open(f, "w").close()
        super().setup_class()

    def test_ban_whitelist_stronger_than_cbb_ban(self):
        ordinaryIp = GenRandomIP()
        whitelistedIp = GenRandomIP()

        self.cbb.add_block(CBB_FLAG_BAN_IP_BASED, whitelistedIp, whitelistedIp, None)
        self.cbb.add_block(CBB_FLAG_BAN_IP_BASED, ordinaryIp, ordinaryIp, None)
        time.sleep(CBB_SYNC_PERIOD + 1)

        with open(self.whitelist, "a") as f:
            f.write(whitelistedIp + "\n")

        self.antirobot.reload_data()

        assert IsCaptchaRedirect(self.send_fullreq(
            REGULAR_SEARCH,
            headers={IP_HEADER: ordinaryIp},
        ))

        assert not IsCaptchaRedirect(self.send_fullreq(
            REGULAR_SEARCH,
            headers={IP_HEADER: whitelistedIp},
        ))

    def test_service_whitelists(self):
        ordinary_ip = GenRandomIP()
        default_whitelisted_ip = GenRandomIP()
        img_whitelisted_ip = GenRandomIP()
        market_whitelisted_ip1 = GenRandomIP()
        market_whitelisted_ip2 = GenRandomIP()

        for ip in (ordinary_ip, default_whitelisted_ip, img_whitelisted_ip, market_whitelisted_ip1, market_whitelisted_ip2):
            self.cbb.add_block(CBB_FLAG_BAN_IP_BASED, ip, ip, None)

        time.sleep(CBB_SYNC_PERIOD + 1)

        with open(self.whitelist, "w") as f:
            f.write(f"{default_whitelisted_ip}\n")

        with open(self.img_whitelist, "w") as f:
            f.write(f"{img_whitelisted_ip}\n")

        with open(self.market_whitelist1, "w") as f:
            f.write(f"{market_whitelisted_ip1}\n")

        with open(self.market_whitelist2, "w") as f:
            f.write(f"{market_whitelisted_ip2}\n")

        self.antirobot.reload_data()

        def ping_img(ip):
            return self.send_fullreq(
                "https://yandex.ru/images/search?text=test",
                headers={IP_HEADER: ip}
            )

        def ping_market(ip):
            return self.send_fullreq(
                "https://market.yandex.ru/search?text=test",
                headers={IP_HEADER: ip}
            )

        assert IsCaptchaRedirect(ping_img(ordinary_ip))
        assert not IsCaptchaRedirect(ping_img(default_whitelisted_ip))
        assert not IsCaptchaRedirect(ping_img(img_whitelisted_ip))
        assert IsCaptchaRedirect(ping_img(market_whitelisted_ip1))

        assert IsCaptchaRedirect(self.antirobot.ping_search(ordinary_ip))
        assert not IsCaptchaRedirect(self.antirobot.ping_search(default_whitelisted_ip))
        assert IsCaptchaRedirect(self.antirobot.ping_search(img_whitelisted_ip))
        assert IsCaptchaRedirect(self.antirobot.ping_search(market_whitelisted_ip1))

        assert IsCaptchaRedirect(ping_market(ordinary_ip))
        assert IsCaptchaRedirect(ping_market(default_whitelisted_ip))
        assert IsCaptchaRedirect(ping_market(img_whitelisted_ip))
        assert not IsCaptchaRedirect(ping_market(market_whitelisted_ip1))
        assert not IsCaptchaRedirect(ping_market(market_whitelisted_ip2))

    def test_whitelisted_in_blocks_can_be_banned_by_regexp(self):
        ip = GenRandomIP()

        self.cbb.add_block(CBB_FLAG_NONBLOCKING, ip, ip, None)
        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*robot.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.send_fullreq(
            "http://yandex.ru/search?text=robot",
            headers={IP_HEADER: ip},
        ))

    def test_whitelisted_in_blocks_can_be_banned_by_ip(self):
        ip = GenRandomIP()

        self.cbb.add_block(CBB_FLAG_NONBLOCKING, ip, ip, None)
        self.cbb.add_block(CBB_FLAG_BAN_IP_BASED, ip, ip, None)
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.send_fullreq(
            REGULAR_SEARCH,
            headers={IP_HEADER: ip},
        ))

    def test_whitelisted_cbb_banned_stat(self):
        whitelistedIp = GenRandomIP()

        self.antirobot.ban(whitelistedIp, "yandsearch", False)
        time.sleep(5)

        metric = self.antirobot.query_metric("cacher_request_whitelisted_deee", service_type="web")

        self.antirobot.send_fullreq(
            "http://yandex.ru/yandsearch?text=whitelisted_banned_web",
            headers={IP_HEADER: whitelistedIp},
        )

        cacher_daemon_log = self.unified_agent.wait_cacher_log_line_with_query('.*whitelisted_banned_web.*')
        assert cacher_daemon_log.catboost_whitelist is True
        assert cacher_daemon_log.cacher_formula_result != 0
        assert self.antirobot.query_metric("cacher_request_whitelisted_deee", service_type="web") == metric + 1

        self.cbb.add_block(CBB_FLAG_BAN_IP_BASED, whitelistedIp, whitelistedIp, None)
        time.sleep(CBB_SYNC_PERIOD + 1)

        self.antirobot.send_fullreq(
            "http://yandex.ru/yandsearch?text=whitelisted_banned_web",
            headers={IP_HEADER: whitelistedIp},
        )

        cacher_daemon_log = self.unified_agent.wait_cacher_log_line_with_query('.*whitelisted_banned_web.*')
        assert cacher_daemon_log.catboost_whitelist is False
        assert cacher_daemon_log.cacher_formula_result == 0
        assert self.antirobot.query_metric("cacher_request_whitelisted_deee", service_type="web") == metric + 1

    def test_with_spravka_banned_by_regexp(self):
        ip = GenRandomIP()
        spravka = GetSpravkaForAddr(self, ip)

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*should_be_banned.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        request = Fullreq(
            "http://yandex.ru/search?text=should_be_banned",
            headers={
                IP_HEADER: ip,
                "Cookie": f"spravka={spravka}",
            },
        )
        # Пришел с чистой справкой, не должны банить MIN_REQUESTS_WITH_SPRAVKA раз
        assert not IsCaptchaRedirect(self.send_request(request))
        passed_cnt = 1

        for i in range(MIN_REQUESTS_WITH_SPRAVKA * 2):
            if not IsCaptchaRedirect(self.send_request(request)):
                passed_cnt += 1
            time.sleep(0.1)

        assert passed_cnt >= MIN_REQUESTS_WITH_SPRAVKA

        # скорее всего уже забанился, но из-за лага синхронизации подождем
        AssertEventuallyTrue(lambda: IsCaptchaRedirect(self.send_request(request)), waitTimeout=10)


class TestCbbCache(AntirobotTestSuite):
    CbbCache = Path.cwd() / "cbb_cache"

    options = {
        "bans_enabled": True,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbApiTimeout": "0.3s",
        "cbb_captcha_re_flag": CBB_FLAG_CAPTCHA_BY_REGEXP,
        "CbbFlagIpBasedIdentificationsBan": CBB_FLAG_BAN_IP_BASED,
        "CbbFlagNonblocking": CBB_FLAG_NONBLOCKING,
        "CbbFlagNotBanned": CBB_FLAG_NOT_BANNED,
        "MinRequestsWithSpravka": MIN_REQUESTS_WITH_SPRAVKA,
        "CbbCacheFile": CbbCache,
        "CbbCachePeriod": CBB_CACHE_PERIOD,
        "cbb_re_flag": CBB_FLAG_MANUAL_RE,
        "cbb_can_show_captcha_flag": CBB_FLAG_CAN_SHOW_CAPTCHA,
        "bans_enabled@mail": False,
    }

    def append_rule(self, rule):
        self.cbb.add_text_block(CBB_FLAG_MANUAL_RE, rule)
        time.sleep(CBB_SYNC_PERIOD + 1)

    def test_regex_on_requests(self):
        self.append_rule(r"request=/.*User-agent:.*Referer:.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        AssertEventuallyBlocked(self, BlockedRequest)
        AssertEventuallyNotBlocked(self, NonBlockedRequest)

    def test_regex_on_robot_set(self):
        ip = GenRandomIP()
        self.antirobot.ban(ip, "yandsearch")
        robot_request = Fullreq("http://yandex.ru/yandsearch?text=i_am_robot_and_i_got_captcha", headers={IP_HEADER: ip})
        assert IsCaptchaRedirect(self.send_request(robot_request))

        self.append_rule(r"in_robot_set=yes")
        time.sleep(CBB_SYNC_PERIOD + 1)

        robot_request = Fullreq("http://yandex.ru/yandsearch?text=i_am_robot_and_i_got_403", headers={IP_HEADER: ip})
        AssertBlocked(self.antirobot.send_request(robot_request))

    def test_may_ban_cbb(self):
        self.cbb.add_text_block(CBB_FLAG_CAN_SHOW_CAPTCHA, r"doc=/.*matchme.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        def check_for_match(can_show_captcha_flag_match, bans_enabled, maybanfor, regexp):
            self.cbb.add_text_block(CBB_FLAG_MANUAL_RE, regexp)
            time.sleep(CBB_SYNC_PERIOD + 1)

            if can_show_captcha_flag_match:
                url = "http://yandex.ru/matchme"
            else:
                url = "http://yandex.ru/no_match"

            if bans_enabled:
                host = "web"
            else:
                host = "mail"

            result = IsBlockedResponse(
                self.send_request(
                    Fullreq(
                        url,
                        headers={
                            IP_HEADER: GenRandomIP(),
                            FORCE_HOST_HEADER: host,
                            MAYBAN_HEADER: maybanfor,
                        }
                    )
                )
            )

            self.cbb.remove_text_block(CBB_FLAG_MANUAL_RE, regexp)
            time.sleep(CBB_SYNC_PERIOD + 1)

            return result

        assert check_for_match(
            can_show_captcha_flag_match=False,
            bans_enabled=False,
            maybanfor="0",
            regexp=r"may_ban=no",
        )

        assert check_for_match(
            can_show_captcha_flag_match=False,
            bans_enabled=False,
            maybanfor="1",
            regexp=r"may_ban=no",
        )

        assert check_for_match(
            can_show_captcha_flag_match=False,
            bans_enabled=True,
            maybanfor="0",
            regexp=r"may_ban=no",
        )

        assert check_for_match(
            can_show_captcha_flag_match=False,
            bans_enabled=True,
            maybanfor="1",
            regexp=r"may_ban=yes",
        )

        assert check_for_match(
            can_show_captcha_flag_match=True,
            bans_enabled=False,
            maybanfor="0",
            regexp=r"may_ban=yes",
        )

        assert check_for_match(
            can_show_captcha_flag_match=True,
            bans_enabled=True,
            maybanfor="1",
            regexp=r"may_ban=yes",
        )

        assert check_for_match(
            can_show_captcha_flag_match=True,
            bans_enabled=True,
            maybanfor="0",
            regexp=r"may_ban=yes",
        )

        assert check_for_match(
            can_show_captcha_flag_match=False,
            bans_enabled=True,
            maybanfor="1",
            regexp=r"may_ban=yes",
        )

    def test_empty_cbb(self):
        ordinaryIp = GenRandomIP()
        robotRequest = Fullreq(
            "http://yandex.ru/search?text=i_am_robot",
            headers={IP_HEADER: GenRandomIP()},
        )

        self.cbb.add_block(
            CBB_FLAG_BAN_IP_BASED, ordinaryIp, ordinaryIp, None
        )

        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*robot.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.antirobot.send_request(robotRequest))
        assert IsCaptchaRedirect(self.send_fullreq(
            REGULAR_SEARCH,
            headers={IP_HEADER: ordinaryIp},
        ))

        time.sleep(CBB_CACHE_PERIOD + 1)

        # останавливаем демон
        self.antirobot.terminate()
        self.cbb.terminate()

        # запускаем демон
        self.antirobots = self.enter(self.start_antirobots(self.options, num_antirobots=1))

        # баны должны работать.
        assert IsCaptchaRedirect(self.antirobots[0].send_request(robotRequest))
        assert IsCaptchaRedirect(self.antirobots[0].send_fullreq(
            REGULAR_SEARCH,
            headers={IP_HEADER: ordinaryIp},
        ))


class TestCbbBadFlagType(AntirobotTestSuite):
    options = {
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "cbb_captcha_re_flag": CBB_FLAG_CAPTCHA_BY_REGEXP,
    }

    def test_cbb_bad_flag_type(self):
        ip = GenRandomIP()

        self.cbb.add_text_block(CBB_FLAG_MANUAL_RE, "cgi=/.*cheburek.*/")
        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, f"cgi=#{CBB_FLAG_MANUAL_RE}")
        cbb_errors_unfixed1 = self.antirobot.query_metric("cbb_errors_deee")
        time.sleep(CBB_SYNC_PERIOD + 1)

        cbb_errors_unfixed2 = self.antirobot.query_metric("cbb_errors_deee")
        assert cbb_errors_unfixed2 > cbb_errors_unfixed1

        self.cbb.clear(flags=[CBB_FLAG_CAPTCHA_BY_REGEXP])
        self.cbb.add_re_block(CBB_FLAG_RE_LIST, "/.*abacaba.*/")
        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, f"cgi=#{CBB_FLAG_RE_LIST}")
        time.sleep(CBB_SYNC_PERIOD + 1)

        assert IsCaptchaRedirect(self.antirobot.send_fullreq(
            "http://yandex.ru/search?text=abacaba",
            headers={IP_HEADER: ip},
        ))

        cbb_errors_fixed1 = self.antirobot.query_metric("cbb_errors_deee")
        time.sleep(CBB_SYNC_PERIOD + 1)
        cbb_errors_fixed2 = self.antirobot.query_metric("cbb_errors_deee")
        assert cbb_errors_fixed1 == cbb_errors_fixed2


class TestWhiteCbb(AntirobotTestSuite):
    options = {
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "cbb_captcha_re_flag": CBB_FLAG_CAPTCHA_BY_REGEXP,
        "CbbFlagNotBanned": CBB_FLAG_NOT_BANNED,
        "DisableBansByFactors": 1,
        "cacher_threshold@autoru": -100000,
    }

    captcha_args = ['--generate', 'correct', '--check', 'success']

    def test_not_enemy_banned_ip(self):
        ip = GenRandomIP()

        self.antirobot.ban(ip)
        time.sleep(5)

        response = self.send_fullreq(
            "http://yandex.ru/yandsearch?text=banned_ip",
            headers={
                IP_HEADER: ip,
            },
        )

        assert IsCaptchaRedirect(response)
        log_line = self.unified_agent.wait_log_line_with_query(r".*banned_ip.*")
        assert log_line["verdict"] == 'ENEMY'

        self.cbb.add_text_block(CBB_FLAG_NOT_BANNED, "cgi=/.*banned_ip.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        response = self.send_fullreq(
            "http://yandex.ru/yandsearch?text=banned_ip",
            headers={
                IP_HEADER: ip,
            },
        )

        assert not IsCaptchaRedirect(response)
        log_line = self.unified_agent.wait_log_line_with_query(r".*banned_ip.*")
        assert log_line["verdict"] == 'NEUTRAL'

    def test_not_enemy_autoru_cacher_formula(self):
        ip = GenRandomIP()
        response = self.send_fullreq(
            "http://auto.ru/search?text=banned_cacher_formula",
            headers={
                IP_HEADER: ip,
                FORCE_HOST_HEADER: "autoru",
                "X-Yandex-ICookie": VALID_I_COOKIE,
            },
        )

        assert IsCaptchaRedirect(response)
        log_line = self.unified_agent.wait_log_line_with_query(r".*banned_cacher_formula.*")
        assert log_line["verdict"] == 'ENEMY'

        self.cbb.add_text_block(CBB_FLAG_NOT_BANNED, "cgi=/.*banned_cacher_formula.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        ip = GenRandomIP()
        response = self.send_fullreq(
            "http://auto.ru/search?text=banned_cacher_formula",
            headers={
                IP_HEADER: ip,
                FORCE_HOST_HEADER: "autoru",
                "X-Yandex-ICookie": VALID_I_COOKIE,
            },
        )

        assert not IsCaptchaRedirect(response)
        log_line = self.unified_agent.wait_log_line_with_query(r".*banned_cacher_formula.*")
        assert log_line["verdict"] == 'NEUTRAL'

    def test_not_enemy_banned_by_cbb(self):
        self.cbb.add_text_block(CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*banned_cbb.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        ip = GenRandomIP()
        response = self.send_fullreq(
            "http://yandex.ru/yandsearch?text=banned_cbb",
            headers={
                IP_HEADER: ip,
                "X-Yandex-ICookie": VALID_I_COOKIE,
            },
        )

        assert IsCaptchaRedirect(response)
        log_line = self.unified_agent.wait_log_line_with_query(r".*banned_cbb.*")
        assert log_line["verdict"] == 'ENEMY'

        self.cbb.add_text_block(CBB_FLAG_NOT_BANNED, "cgi=/.*banned_cbb.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        ip = GenRandomIP()
        response = self.send_fullreq(
            "http://yandex.ru/yandsearch?text=banned_cbb",
            headers={
                IP_HEADER: ip,
                "X-Yandex-ICookie": VALID_I_COOKIE,
            },
        )

        assert not IsCaptchaRedirect(response)
        log_line = self.unified_agent.wait_log_line_with_query(r".*banned_cbb.*")
        assert log_line["verdict"] == 'NEUTRAL'
