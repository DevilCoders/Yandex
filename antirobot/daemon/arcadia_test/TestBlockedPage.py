import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    GenRandomIP,
)
from antirobot.daemon.arcadia_test.util.asserts import AssertBlocked
from antirobot.daemon.arcadia_test import util


BLOCKED_RUS = 'Доступ к&nbsp;нашему сервису временно запрещён'
BLOCKED_ENG = 'Access to&nbsp;our service has been temporarily blocked'
BLOCKED_UKR = 'Доступ к&nbsp;нашему сервису временно запрещён'
BLOCKED_BEL = 'Доступ к&nbsp;нашему сервису временно запрещён'
BLOCKED_TUR = 'Servise erişim geçici olarak engellendi'
BLOCKED_KAZ = 'Сервисімізге қосылуға уақытша тыйым салынған'
BLOCKED_TAT = 'Доступ к&nbsp;нашему сервису временно запрещён'
BLOCKED_UZB = 'Bizning xizmatimizga kirish vaqtincha taqiqlangan'


class TestBlockedPage(AntirobotTestSuite):
    @classmethod
    def setup_class(cls):
        super().setup_class()
        cls.blocked_ip = GenRandomIP()
        cls.antirobot.block(cls.blocked_ip)

    @pytest.mark.parametrize("host, fragment", [
        ("yandex.com",   'jM3MDJINTUuMzQ4NVoiIGZpbGw9IiNGQzNGMUQiPjwvcGF0aD48L3N2Zz4='),
        ("yandex.eu",    'jM3MDJINTUuMzQ4NVoiIGZpbGw9IiNGQzNGMUQiPjwvcGF0aD48L3N2Zz4='),
        ("yandex.ru",    'Y5MUg0NC4xMzM3Vjg5LjIzNDZaIiBmaWxsPSIjRkMzRjFEIj48L3BhdGg+PC9zdmc+'),
        ("auto.ru",      'bD0iI0RCMzcyNyIgZmlsbC1ydWxlPSJldmVub2RkIi8+PC9zdmc+'),
        ("kinopoisk.ru", 'wMCIvPgo8L3JhZGlhbEdyYWRpZW50Pgo8L2RlZnM+Cjwvc3ZnPg=='),
    ])
    def test_logo_image(self, host, fragment):
        request = r"http://{}/search".format(host)
        resp = self.send_request(Fullreq(req=request, headers={"X-Forwarded-For-Y": self.blocked_ip}))
        AssertBlocked(resp)
        assert fragment in resp.read().decode(), 'Page is expected to contain "{}"'.format(fragment)

    def test_has_metrika_script(self):
        request = "https://yandex.ru/search"
        resp = self.send_request(Fullreq(req=request, headers={"X-Forwarded-For-Y": self.blocked_ip}))
        assert resp.getcode() == 403
        blocked_page = resp.read().decode()
        assert "mc.yandex.ru/watch/15897442" in blocked_page

    @pytest.mark.parametrize("host, fragment", [
        ("yandex.com",    BLOCKED_ENG),
        ("yandex.eu",    BLOCKED_ENG),
        ("yandex.ru",     BLOCKED_RUS),
        ("yandex.ua",     BLOCKED_UKR),
        ("yandex.by",     BLOCKED_BEL),
        ("yandex.com.tr", BLOCKED_TUR),
        ("yandex.kz",     BLOCKED_KAZ),
        ("yandex.uz",     BLOCKED_UZB),
        ("auto.ru",       BLOCKED_RUS),
        ("kinopoisk.ru",  BLOCKED_RUS),
    ])
    def test_localization_by_tld(self, host, fragment):
        self.common_test_localization(host, None, fragment)

    @pytest.mark.parametrize("cookie, fragment", [
        (util.MY_COOKIE_ENG, BLOCKED_ENG),
        (util.MY_COOKIE_RUS, BLOCKED_RUS),
        (util.MY_COOKIE_UKR, BLOCKED_UKR),
        (util.MY_COOKIE_BEL, BLOCKED_BEL),
        (util.MY_COOKIE_TUR, BLOCKED_TUR),
        (util.MY_COOKIE_KAZ, BLOCKED_KAZ),
        (util.MY_COOKIE_UZB, BLOCKED_UZB),
        (util.MY_COOKIE_TAT, BLOCKED_TAT),
    ])
    def test_localization_by_my_cookie(self, cookie, fragment):
        if cookie == util.MY_COOKIE_ENG:
            host = "yandex.ru"
        else:
            host = "yandex.com"

        self.common_test_localization(host, cookie, fragment)

    def common_test_localization(self, host, cookie, fragment):
        request = r"http://{}/search".format(host)
        headers = {"X-Forwarded-For-Y": self.blocked_ip}

        if cookie is not None:
            headers["Cookie"] = "my=" + cookie

        resp = self.send_request(Fullreq(req=request, headers=headers))

        AssertBlocked(resp)
        assert fragment in resp.read().decode(), 'Page is expected to contain "{}"'.format(fragment)

    def test_cache_control(self):
        request = "https://yandex.ru/search"
        resp = self.send_request(Fullreq(req=request, headers={"X-Forwarded-For-Y": self.blocked_ip}))
        assert resp.getcode() == 403
        assert resp.headers["Cache-Control"] == "public, max-age=120, immutable"
