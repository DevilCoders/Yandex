import time
from pathlib import Path

import pytest

from antirobot.daemon.arcadia_test.util import (
    GenRandomIP,
    Fullreq,
    IsBlockedResponse,
    IsCaptchaRedirect,
    VALID_I_COOKIE,
    IP_HEADER,
)
from antirobot.daemon.arcadia_test.util import asserts
from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite

CBB_SYNC_PERIOD = 1
CBB_FLAG_MANUAL_RE = 11
CBB_FLAG_CAPTCHA_BY_REGEXP = 13
CBB_FLAG_BAN_BY_IP = 255
DDOSER_MIKROTIK_SQUID_IP = "91.28.135.0"
DDOSER_MIKROTIK_IP = "91.28.135.1"


class BaseTestDisabling(AntirobotTestSuite):
    Controls = Path.cwd() / "controls"
    HandleStopBlockForAllFilePath = Controls / "stop_block_for_all"
    HandleStopBanForAllFilePath = Controls / "stop_ban_for_all"
    HandleStopBlockFilePath = Controls / "stop_block"
    HandleStopBanFilePath = Controls / "stop_ban"
    HandleAllowMainBanFilePath = Controls / "allow_main_ban"
    HandleAllowDzenSearchBanFilePath = Controls / "allow_dzensearch_ban"
    HandleAllowBanAllFilePath = Controls / "allow_ban_all"
    HandleAllowShowCaptchaAllFilePath = Controls / "allow_show_captcha_all"
    HandlePreviewIdentTypeEnabledFilePath = Controls / "preview_ident_type_enabled"
    HandleCbbPanicModePath = Controls / "cbb_panic_mode"

    @classmethod
    def setup_class(cls):
        cls.Controls.mkdir(exist_ok=True)
        super().setup_class()

    def setup_method(self, method):
        super().setup_method(method)

        for file_path in (self.HandleStopBlockForAllFilePath, self.HandleStopBlockFilePath, self.HandleStopBanForAllFilePath, self.HandleStopBanFilePath,
                          self.HandleAllowMainBanFilePath, self.HandleAllowBanAllFilePath, self.HandleAllowShowCaptchaAllFilePath,
                          self.HandlePreviewIdentTypeEnabledFilePath, self.HandleCbbPanicModePath, self.HandleAllowDzenSearchBanFilePath):
            with open(file_path, "w") as file:
                file.write("")


class TestDisabling(BaseTestDisabling):
    options = {
        "cbb_captcha_re_flag": CBB_FLAG_CAPTCHA_BY_REGEXP,
        "cbb_re_flag": CBB_FLAG_MANUAL_RE,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbFlagIpBasedIdentificationsBan": CBB_FLAG_BAN_BY_IP,
        "HandleWatcherPollInterval": "0.5s",
        "HandleStopBlockFilePath": BaseTestDisabling.HandleStopBlockFilePath,
        "HandleStopBanFilePath": BaseTestDisabling.HandleStopBanFilePath,
        "HandleStopBlockForAllFilePath": BaseTestDisabling.HandleStopBlockForAllFilePath,
        "HandleStopBanForAllFilePath": BaseTestDisabling.HandleStopBanForAllFilePath,
        "HandleAllowMainBanFilePath": BaseTestDisabling.HandleAllowMainBanFilePath,
        "HandleAllowDzenSearchBanFilePath": BaseTestDisabling.HandleAllowDzenSearchBanFilePath,
        "HandleAllowBanAllFilePath": BaseTestDisabling.HandleAllowBanAllFilePath,
        "HandleAllowShowCaptchaAllFilePath": BaseTestDisabling.HandleAllowShowCaptchaAllFilePath,
        "HandlePreviewIdentTypeEnabledFilePath": BaseTestDisabling.HandlePreviewIdentTypeEnabledFilePath,
        "HandleCbbPanicModePath": BaseTestDisabling.HandleCbbPanicModePath,
        "DisableBansByFactors": 1,
        "InitialChinaRedirectEnabled": False,
    }

    def block_all(self, ip, service):
        flag = int(self.antirobot.dump_cfg(service)["CbbFlagsManual"])
        self.cbb.add_block(flag, ip, ip, None)

    def block_all_web_and_wait(self, ip):
        self.block_all(ip, "web")
        asserts.AssertEventuallyTrue(
            lambda: self.is_blocked_web(ip)
        )  # Should be blocked eventually by CBB

    def is_blocked_web(self, ip):
        return IsBlockedResponse(
            self.antirobot.send_request(
                Fullreq(
                    "http://yandex.ru/yandsearch?text=some_text",
                    headers={IP_HEADER: ip},
                )
            )
        )

    def is_blocked_img(self, ip):
        return IsBlockedResponse(
            self.antirobot.send_request(
                Fullreq(
                    "http://yandex.ru/images/search?text=some_text",
                    headers={IP_HEADER: ip},
                )
            )
        )

    def is_banned_web(self, ip):
        return IsCaptchaRedirect(
            self.antirobot.send_fullreq(
                "http://yandex.ru/yandsearch?text=some_text",
                headers={IP_HEADER: ip},
            )
        )

    def is_banned_img(self, ip):
        return IsCaptchaRedirect(
            self.antirobot.send_fullreq(
                "http://yandex.ru/images/search?text=some_text",
                headers={IP_HEADER: ip},
            )
        )

    def test_panic_can_show_captcha(self):
        ip = GenRandomIP()
        otherRequest = Fullreq(
            "http://yandex.ru/nonexistent", headers={IP_HEADER: ip}
        )

        self.antirobot.ban(ip)
        asserts.AssertNotRobotResponse(self.send_request(otherRequest))

        with open(self.HandleAllowShowCaptchaAllFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(self.send_request(otherRequest))
        )

        with open(self.HandleAllowShowCaptchaAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(
            lambda: not IsCaptchaRedirect(self.send_request(otherRequest))
        )

    @pytest.mark.parametrize('url, service, is_morda', [
        ("https://yandex.ru/", "morda", True),   # morda
        ("https://yandex.ru/portal/dzensearch/desktop", "morda", False),  # dzensearch
        ("https://ya.ru/portal/dzensearch/mobile", "yaru", False),   # dzensearch
    ])
    def test_panic_main_only(self, url, service, is_morda):
        its = self.HandleAllowMainBanFilePath if is_morda else self.HandleAllowDzenSearchBanFilePath
        ip = GenRandomIP()
        mordaRequest = Fullreq(
            url,
            headers={
                IP_HEADER: ip,
                "X-Yandex-ICookie": VALID_I_COOKIE,
            }
        )
        bannedMordaRequest = Fullreq(
            url + "?i_am_robot",
            headers={
                IP_HEADER: ip,
                "X-Yandex-ICookie": VALID_I_COOKIE,
            }
        )

        with open(its, "w") as file:
            file.write(service)

        self.cbb.add_text_block(
            CBB_FLAG_CAPTCHA_BY_REGEXP, "cgi=/.*i_am_robot.*/"
        )

        asserts.AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(
                self.antirobot.send_request(bannedMordaRequest)
            )
        )

        with open(its, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(
            lambda: not IsCaptchaRedirect(self.send_request(mordaRequest))
        )

        with open(its, "w") as file:
            file.write(service)

        asserts.AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(self.antirobot.send_request(mordaRequest))
        )

        with open(its, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(
            lambda: not IsCaptchaRedirect(self.antirobot.send_request(mordaRequest))
        )

    def test_panic_may_ban_for_status_metric(self):
        assert (
            self.antirobot.get_metric(
                "service_type=web;panic_maybanfor_status_ahhh"
            )
            == 0
        )

        with open(self.HandleAllowBanAllFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(
            lambda: self.antirobot.get_metric(
                "service_type=web;panic_maybanfor_status_ahhh"
            )
            == 1
        )

        with open(self.HandleAllowBanAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(
            lambda: self.antirobot.get_metric(
                "service_type=web;panic_maybanfor_status_ahhh"
            )
            == 0
        )

    def test_panic_can_show_captcha_status_metric(self):
        assert (
            self.antirobot.get_metric(
                "service_type=web;panic_canshowcaptcha_status_ahhh"
            )
            == 0
        )

        with open(self.HandleAllowShowCaptchaAllFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(
            lambda: self.antirobot.get_metric(
                "service_type=web;panic_canshowcaptcha_status_ahhh"
            )
            == 1
        )

        with open(self.HandleAllowShowCaptchaAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(
            lambda: self.antirobot.get_metric(
                "service_type=web;panic_canshowcaptcha_status_ahhh"
            )
            == 0
        )

    def test_panic_main_only_status_metric(self):
        assert (
            self.antirobot.get_metric("service_type=web;panic_morda_status_ahhh")
            == 0
        )

        with open(self.HandleAllowMainBanFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(
            lambda: self.antirobot.get_metric(
                "service_type=web;panic_morda_status_ahhh"
            )
            == 1
        )

        with open(self.HandleAllowMainBanFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(
            lambda: self.antirobot.get_metric(
                "service_type=web;panic_morda_status_ahhh"
            )
            == 0
        )

    def test_panic_never_ban_status_metric(self):
        assert (
            self.antirobot.get_metric("service_type=web;panic_neverban_status_ahhh")
            == 0
        )

        with open(self.HandleStopBanFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(
            lambda: self.antirobot.get_metric(
                "service_type=web;panic_neverban_status_ahhh"
            )
            == 1
        )

        with open(self.HandleStopBanFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(
            lambda: self.antirobot.get_metric(
                "service_type=web;panic_neverban_status_ahhh"
            )
            == 0
        )

    def test_panic_never_block_status_metric(self):
        assert (
            self.antirobot.get_metric(
                "service_type=web;panic_neverblock_status_ahhh"
            )
            == 0
        )

        with open(self.HandleStopBlockFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(
            lambda: self.antirobot.get_metric(
                "service_type=web;panic_neverblock_status_ahhh"
            )
            == 1
        )

        with open(self.HandleStopBlockFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(
            lambda: self.antirobot.get_metric(
                "service_type=web;panic_neverblock_status_ahhh"
            )
            == 0
        )

    def test_no_block_with_persistent_handle(self):
        ip = GenRandomIP()
        self.block_all_web_and_wait(ip)

        with open(self.HandleStopBlockForAllFilePath, "w") as file:
            file.write("enable")

        asserts.AssertEventuallyTrue(lambda: not self.is_blocked_web(ip))

        with open(self.HandleStopBlockForAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: self.is_blocked_web(ip))

    def test_no_block_service_handle_with_persistent_handle(self):
        ip = GenRandomIP()
        self.block_all_web_and_wait(ip)

        with open(self.HandleStopBlockForAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: self.is_blocked_web(ip))

        with open(self.HandleStopBlockFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(lambda: not self.is_blocked_web(ip))

    def test_check_disable_total_block_with_enable_service_block_with_persistent_handle(self):
        ip = GenRandomIP()
        self.block_all_web_and_wait(ip)

        with open(self.HandleStopBlockForAllFilePath, "w") as file:
            file.write("enable")
        with open(self.HandleStopBlockFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: not self.is_blocked_web(ip))

        with open(self.HandleStopBlockForAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: self.is_blocked_web(ip))

    def test_check_disable_total_block_with_disable_service_block_with_persistent_handle(self):
        ip = GenRandomIP()
        self.block_all_web_and_wait(ip)

        with open(self.HandleStopBlockFilePath, "w") as file:
            file.write("web")
        with open(self.HandleStopBlockForAllFilePath, "w") as file:
            file.write("enable")

        asserts.AssertEventuallyTrue(lambda: not self.is_blocked_web(ip))

        with open(self.HandleStopBlockForAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: not self.is_blocked_web(ip))

    def test_no_block_with_two_services_with_persistent_handle(self):
        ip = GenRandomIP()
        self.block_all_web_and_wait(ip)

        with open(self.HandleStopBlockForAllFilePath, "w") as file:
            file.write("")
        with open(self.HandleStopBlockFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(lambda: not self.is_blocked_web(ip))

        asserts.AssertEventuallyTrue(lambda: self.is_blocked_img(ip))

    def test_no_ban_with_persistent_handle(self):
        ip = GenRandomIP()
        self.antirobot.ban(ip)
        with open(self.HandleStopBanForAllFilePath, "w") as file:
            file.write("enable")

        asserts.AssertEventuallyTrue(lambda: not self.antirobot.is_banned(ip))

        with open(self.HandleStopBanForAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: self.antirobot.is_banned(ip))

    def test_no_ban_service_handle_with_persistent_handle(self):
        ip = GenRandomIP()
        self.antirobot.ban(ip)

        with open(self.HandleStopBanForAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: self.is_banned_web(ip))

        with open(self.HandleStopBanFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(lambda: not self.is_banned_web(ip))

    def test_check_disable_total_ban_with_enable_service_ban_with_persistent_handle(self):
        ip = GenRandomIP()
        self.antirobot.ban(ip)

        with open(self.HandleStopBanForAllFilePath, "w") as file:
            file.write("enable")
        with open(self.HandleStopBanFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: not self.is_banned_web(ip))

        with open(self.HandleStopBanForAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: self.is_banned_web(ip))

    def test_check_disable_total_ban_with_disable_service_ban_with_persistent_handle(self):
        ip = GenRandomIP()
        self.antirobot.ban(ip)

        with open(self.HandleStopBanForAllFilePath, "w") as file:
            file.write("enable")
        with open(self.HandleStopBanFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(lambda: not self.is_banned_web(ip))

        with open(self.HandleStopBanForAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: not self.is_banned_web(ip))

    def test_no_ban_with_two_services_with_persistent_handle(self):
        ip = GenRandomIP()
        self.antirobot.ban(ip, "yandsearch")
        self.antirobot.ban(ip, "images/search")

        with open(self.HandleStopBanForAllFilePath, "w") as file:
            file.write("")
        with open(self.HandleStopBanFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(lambda: not self.is_banned_web(ip))

        asserts.AssertEventuallyTrue(lambda: self.is_banned_img(ip))

    def test_no_ban_service_handle_with_cbb(self):
        ip = GenRandomIP()
        self.cbb.add_block(CBB_FLAG_BAN_BY_IP, ip, ip, None)

        with open(self.HandleStopBanForAllFilePath, "w") as file:
            file.write("")

        asserts.AssertEventuallyTrue(lambda: self.is_banned_web(ip))

        with open(self.HandleStopBanFilePath, "w") as file:
            file.write("web")

        asserts.AssertEventuallyTrue(lambda: not self.is_banned_web(ip))

    def test_no_block_metric_persistent_403(self):
        old_value = self.antirobot.get_metric("not_blocked_requests_deee")

        ip = GenRandomIP()
        self.block_all_web_and_wait(ip)

        with open(self.HandleStopBlockForAllFilePath, "w") as file:
            file.write("enable")

        asserts.AssertEventuallyTrue(lambda: not self.antirobot.is_blocked(ip))

        assert self.antirobot.get_metric("not_blocked_requests_deee") > old_value, \
            "Should be some not blocked requests"

    def test_no_block_metric_persistent_302(self):
        ip = GenRandomIP()
        self.antirobot.ban(ip)

        with open(self.HandleStopBanForAllFilePath, "w") as file:
            file.write("enable")

        old_value = self.antirobot.get_metric("not_banned_requests_deee")

        asserts.AssertEventuallyTrue(lambda: not self.antirobot.is_banned(ip))

        assert self.antirobot.get_metric("not_banned_requests_deee") > old_value, \
            "Should be some not banned requests"

    def test_cbb_panic_mode(self):
        assert(self.antirobot.get_metric("service_type=web;panic_cbb_status_ahhh") == 0)
        ip = GenRandomIP()

        req_web = Fullreq("http://yandex.ru/search?text=test", headers={"X-Forwarded-For-Y": ip})
        req_market = Fullreq("http://market.yandex.ru/shop", headers={"X-Forwarded-For-Y": ip})

        # изначально не заблокирован
        asserts.AssertEventuallyNotBlocked(self, req_web)

        self.cbb.add_text_block(CBB_FLAG_MANUAL_RE, r'panic_mode=yes')
        time.sleep(CBB_SYNC_PERIOD + 1)
        # после добавления в ЕББ всё ещё не заблокирован
        asserts.AssertEventuallyNotBlocked(self, req_web)
        asserts.AssertEventuallyNotBlocked(self, req_market)

        with open(self.HandleCbbPanicModePath, "w") as file:
            file.write("web")

        # после включения ручки web должен заблокироваться
        asserts.AssertEventuallyBlocked(self, req_web)
        asserts.AssertEventuallyNotBlocked(self, req_market)
        assert (self.antirobot.get_metric("service_type=web;panic_cbb_status_ahhh") == 1)

        with open(self.HandleCbbPanicModePath, "w") as file:
            file.write("web,market")

        asserts.AssertEventuallyBlocked(self, req_web)
        asserts.AssertEventuallyBlocked(self, req_market)

        with open(self.HandleCbbPanicModePath, "w") as file:
            file.write("market")

        # после отключения должен разблокироваться
        asserts.AssertEventuallyNotBlocked(self, req_web)
        asserts.AssertEventuallyBlocked(self, req_market)
        assert (self.antirobot.get_metric("service_type=web;panic_cbb_status_ahhh") == 0)
        assert (self.antirobot.get_metric("service_type=market;panic_cbb_status_ahhh") == 1)

    def test_ip_dict(self):
        # NOTE: IP-шники размечены в "test_data" в "global_config.json"
        assert not self.is_blocked_web(DDOSER_MIKROTIK_SQUID_IP)
        assert self.unified_agent.wait_log_line_with_query(".*")["mini_geobase_mask"] == 7

        self.cbb.add_text_block(CBB_FLAG_MANUAL_RE, r"is_squid=yes")
        time.sleep(CBB_SYNC_PERIOD + 1)
        assert self.is_blocked_web(DDOSER_MIKROTIK_SQUID_IP)
        assert self.unified_agent.wait_log_line_with_query(".*")["mini_geobase_mask"] == 7

        assert not self.is_blocked_web(DDOSER_MIKROTIK_IP)
        assert self.unified_agent.wait_log_line_with_query(".*")["mini_geobase_mask"] == 5

        self.cbb.add_text_block(CBB_FLAG_MANUAL_RE, r"is_ddoser=yes")
        time.sleep(CBB_SYNC_PERIOD + 1)
        assert self.is_blocked_web(DDOSER_MIKROTIK_SQUID_IP)
        assert self.is_blocked_web(DDOSER_MIKROTIK_IP)


class TestDisablingPanicMayBanFor(BaseTestDisabling):
    options = {
        "bans_enabled": False,
        "HandleAllowBanAllFilePath": BaseTestDisabling.HandleAllowBanAllFilePath,
        "HandleWatcherPollInterval": "0.5s",
    }

    def test_panic_may_ban_for(self):
        ip = GenRandomIP()

        self.antirobot.ban(ip, check=False)
        time.sleep(2)
        assert not self.antirobot.is_banned(ip)

        with open(self.HandleAllowBanAllFilePath, "w") as file:
            file.write("web")

        self.antirobot.ban(ip)
