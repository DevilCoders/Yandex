import pytest
import urllib
import time
import re

from pathlib import Path
from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    IsCaptchaRedirect,
    GenRandomIP,
    VALID_I_COOKIE,
)
from antirobot.daemon.arcadia_test.util.captcha_page import FORCED_CAPTCHA_REQUEST
from antirobot.daemon.arcadia_test.util.asserts import (
    AssertCaptchaRedirect,
    AssertJsonCaptchaResponse,
)

IP_HEADER = "X-Forwarded-For-Y"
ANTIROBOT_SERVICE_HEADER = "X-Antirobot-Service-Y"

YANDEX_IP = "179.252.199.229"

CBB_SYNC_PERIOD = 1
CBB_FLAG_NOT_BANNED = 499


class TestCaptchaRequest(AntirobotTestSuite):
    options = {
        "bans_enabled": True,
        "cacher_threshold": -100000
    }

    captcha_args = ['--generate', 'correct']

    @pytest.mark.parametrize("host, path, cgi, asserter", [
        ("yandex.ru", "/yandsearch", "", AssertCaptchaRedirect),
        ("yandex.ru", "/yandsearch", "ajax=1", AssertJsonCaptchaResponse),

        ("news.yandex.ru", "/search", "", AssertCaptchaRedirect),
        ("news.yandex.ru", "/search", "format=json", AssertJsonCaptchaResponse),
        ("news.yandex.ru", "/search", "ajax=1", AssertJsonCaptchaResponse),
    ])
    def test_always_returns_captcha(self, host, path, cgi, asserter):
        request = urllib.request.Request("http://" + host + path + "?" + cgi + "&text=%s" % FORCED_CAPTCHA_REQUEST)
        asserter(self.send_request(Fullreq(request)))

    @pytest.mark.parametrize("host, path, cgi", [
        ("yandex.ru",      "/search/touch",           "txt=123"),
        ("yandex.ru",      "/msearch",                ""),
        ("yandex.ru",      "/search/smart",           ""),
        ("yandex.ru",      "/smartsearch",            "lr=21"),
        ("yandex.ru",      "/touchsearchSOME/TEXT",   ""),

        ("yandex.com",     "/search/touch",           "cgi=some_cgi"),
        ("yandex.eu",      "/search/touch",           "cgi=some_cgi"),
        ("yandex.com.tr",  "/search/touch",           ""),

        ("m.yandex.ru",    "/yandsearch",             ""),
        ("m.autoru.ru",    "/some_path",              ""),
        ("m.kinopoisk.ru", "/some_path",              "cgi=some_cgi"),

        ("yandex.ru",      "/yandsearch",             ""),
        ("yandex.com",     "/yandsearch",             ""),
        ("yandex.eu",      "/yandsearch",             ""),
        ("yandex.com.tr",  "/yandsearch",             ""),
        ("autoru.com",     "/some_path",              ""),
        ("kinopoisk.ru",   "/some_path",              "cgi=some_cgi"),
    ])
    def test_returns_correct_captcha(self, host, path, cgi):
        request = urllib.request.Request("http://" + host + path + "?" + cgi + "&text=%s" % FORCED_CAPTCHA_REQUEST)
        resp = self.send_request(Fullreq(request, headers={'X-Antirobot-MayBanFor-Y': '1'}))
        AssertCaptchaRedirect(resp)
        page = self.send_request(Fullreq(resp.info()["Location"])).read().decode()
        assert re.search("captcha_smart.\\w+.min.css", page)


class TestAutoRuCacherFormula(AntirobotTestSuite):
    DataDir = Path.cwd()
    YandexIps = DataDir / "yandex_ips"

    options = {
        "cacher_threshold": -100000,
        "YandexIpsFile": YandexIps.name,
        "YandexIpsDir": DataDir,
        "CbbFlagNotBanned": CBB_FLAG_NOT_BANNED,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "DisableBansByFactors": 1,
    }

    captcha_args = ['--generate', 'correct', '--check', 'success']

    def setup_class(cls):
        with open(cls.YandexIps, "w") as f:
            print(YANDEX_IP, file=f)
        super().setup_class()

    def send_autoru_req(self, ip):
        response = self.send_fullreq(
            "http://auto.ru/search?text=autoru_request",
            headers={
                IP_HEADER: ip,
                ANTIROBOT_SERVICE_HEADER: "autoru",
                "X-Yandex-ICookie": VALID_I_COOKIE,
            },
        )
        return response

    def test_autoru_cacher_formula_ban(self):
        assert IsCaptchaRedirect(self.send_autoru_req(GenRandomIP()))
        log_line = self.unified_agent.wait_log_line_with_query(r".*autoru_request.*")
        assert log_line["verdict"] == 'ENEMY'

    def test_autoru_cacher_formula_not_ban(self):
        assert not IsCaptchaRedirect(self.send_autoru_req(YANDEX_IP))
        log_line = self.unified_agent.wait_log_line_with_query(r".*autoru_request.*")
        assert log_line["verdict"] == 'NEUTRAL'

        self.cbb.add_text_block(CBB_FLAG_NOT_BANNED, "cgi=/.*must_not_be_banned.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        ip = GenRandomIP()
        response = self.send_fullreq(
            "http://auto.ru/search?text=must_not_be_banned",
            headers={
                IP_HEADER: ip,
                ANTIROBOT_SERVICE_HEADER: "autoru",
                "X-Yandex-ICookie": VALID_I_COOKIE,
            },
        )

        assert not IsCaptchaRedirect(response)
        log_line = self.unified_agent.wait_log_line_with_query(r".*must_not_be_banned.*")
        assert log_line["verdict"] == 'NEUTRAL'


class TestBanRequest(AntirobotTestSuite):
    options = {
        "ios_version_supports_captcha@apiauto": "20.5.5",
        "android_version_supports_captcha@apiauto": "16.5.5"
    }
    captcha_args = ['--generate', 'correct', '--check', 'success']

    @pytest.mark.parametrize("os, version, is_ban_req", [
        ("x-ios-app-version", "16.6.6", False),
        ("x-ios-app-version", "20.3.6", False),
        ("x-ios-app-version", "20.5.4", False),
        ("x-ios-app-version", "21.3.2", True),
        ("x-ios-app-version", "20.6.2", True),
        ("x-ios-app-version", "20.5.5", True),
        ("x-ios-app-version", "20.5.6", True),
        ("x-android-app-version", "12.6.6", False),
        ("x-android-app-version", "16.3.6", False),
        ("x-android-app-version", "16.5.4", False),
        ("x-android-app-version", "17.3.2", True),
        ("x-android-app-version", "16.6.2", True),
        ("x-android-app-version", "16.5.5", True),
        ("x-android-app-version", "16.5.6", True),
    ])
    def test_apiauto_ban(self, os, version, is_ban_req):
        response = self.send_fullreq(
            "http://autoru.ru/search?ajax=1",
            headers={
                IP_HEADER: GenRandomIP(),
                ANTIROBOT_SERVICE_HEADER: "apiauto",
                os: version,
                'X-Antirobot-MayBanFor-Y': '1'
            },
        )
        try:
            AssertJsonCaptchaResponse(response, code=401)
        except AssertionError:
            assert not is_ban_req
