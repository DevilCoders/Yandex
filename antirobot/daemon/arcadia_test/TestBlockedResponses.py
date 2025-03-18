import json
import time
import urllib.request
from xml.etree import ElementTree

import pytest
from pathlib import Path

from antirobot.daemon.arcadia_test.util.asserts import (
    AssertEventuallyTrue,
    AssertNotRobotResponse,
)
from antirobot.daemon.arcadia_test.util import Fullreq, GenRandomIP, VALID_L_COOKIE
from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite

ANTIROBOT_SERVICE_HEADER = "X-Antirobot-Service-Y"
IP_HEADER = "X-Forwarded-For-Y"
REAL_IP_HEADER = "X-Real-Ip"
CBB_USER_MARK_FLAG_WEB = 10
CBB_USER_MARK_FLAG_MARKET = 11
CBB_SYNC_PERIOD = 1
YANDEX_IP = "179.252.199.229"
WHITELIST_IP = "172.31.18.234"
REMOVE_EXPIRED_PERIOD = 1
SYNC_LAG = 5
AMNESTY_INTERVAL_SHORT = 30


def fullreq_for_service(service, blockedIp):
    headers = {ANTIROBOT_SERVICE_HEADER: service} if service else {}
    headers[IP_HEADER] = blockedIp
    return Fullreq(
        r"http://yandex.ru/yandsearch?text=a17c4f36e72208c19d13d46a1408945f",
        headers=headers,
    )


class TestBlockedResponses(AntirobotTestSuite):

    DataDir = Path.cwd()
    YandexIps = DataDir / "yandex_ips"
    Whitelist = DataDir / "whitelist_ips"

    options = {
        "cbb_re_user_mark_flag@web": CBB_USER_MARK_FLAG_WEB,
        "cbb_re_user_mark_flag@market": CBB_USER_MARK_FLAG_MARKET,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "YandexIpsFile": YandexIps.name,
        "YandexIpsDir": DataDir,
        "WhiteList": Whitelist.name,
        "RemoveExpiredPeriod": REMOVE_EXPIRED_PERIOD,
        "AmnestyLCookieInterval": AMNESTY_INTERVAL_SHORT,
    }

    captcha_args = ["--generate", "correct"]

    def setup_class(cls):
        with open(cls.YandexIps, "w") as f:
            print(YANDEX_IP, file=f)
        with open(cls.Whitelist, "w") as f:
            print(WHITELIST_IP, file=f)
        super().setup_class()

    def assert_data_is_json_captcha(self, data, redirectUrl):
        vals = json.loads(data)
        assert "type" in vals
        assert vals["type"] == "captcha"
        assert "captcha" in vals
        vals = vals["captcha"]
        assert "captcha-page" in vals
        assert vals["captcha-page"] == redirectUrl

    def block_ip(self, ip=None):
        internalIp = ip if ip is not None else GenRandomIP()
        self.antirobot.block(internalIp)
        return internalIp

    def test_blocking_request(self):
        """
        Этот тест проверяет, что запрос [a17c4f36e72208c19d13d46a1408945f] действительно блокирует
        задавшего его пользователя. Этот запрос мы иногда отдаём внешним людям для тестирования
        интеграции с антироботом.

        https://wiki.yandex-team.ru/jandekspoisk/sepe/antirobotonline/docs/specs/#blokirujushhijjzapros
        """
        blockedIp = self.block_ip()
        self.antirobot.send_request(
            Fullreq(
                r"http://yandex.ru/yandsearch?text=a17c4f36e72208c19d13d46a1408945f",
                headers={IP_HEADER: blockedIp}

            )
        )

        searchReq = Fullreq(r"http://yandex.ru/yandsearch?text=123", headers={IP_HEADER: blockedIp})
        AssertEventuallyTrue(
            lambda: self.antirobot.send_request(searchReq).getcode() == 403,
        )

    def test_ajax_search_json(self):
        blockedIp = self.block_ip()

        request = r"http://yandex.ru/yandsearch?text=123"
        ajaxRequest = request + r"&ajax=1&format=json"

        resp = self.antirobot.send_request(
            Fullreq(ajaxRequest, headers={IP_HEADER: blockedIp})
        )
        assert resp.getcode() == 200
        self.assert_data_is_json_captcha(resp.read(), request)

        self.check_ordinary_block(request, blockedIp)

    def test_ajax_search_json_large_url(self):
        blockedIp = self.block_ip()

        largeText = "12345678" * 200
        request = r"http://yandex.ru/yandsearch?text=" + largeText
        ajaxRequest = request + r"&ajax=1&format=json"

        resp = self.antirobot.send_request(
            Fullreq(ajaxRequest, headers={IP_HEADER: blockedIp})
        )
        assert resp.getcode() == 200
        content = resp.read().decode()

        assert content.count('"img-url"') == 1
        assert content.count('"key"') == 1
        assert content.count('"status"') == 1
        assert content.count('"captcha-page"') == 1

    def test_ajax_search_jsonp(self):
        blockedIp = self.block_ip()

        callback = "cb98671234"
        request = r"http://yandex.ru/yandsearch?text=123"
        ajaxRequest = request + "&callback=%s&ajax=1" % callback

        resp = self.antirobot.send_request(
            Fullreq(ajaxRequest, headers={IP_HEADER: blockedIp})
        )
        assert resp.getcode() == 200
        data = resp.read().decode()
        expectedPrefix = "/**/ " + callback + "("
        assert data.startswith(expectedPrefix)
        assert data.endswith(")")
        self.assert_data_is_json_captcha(
            data[len(expectedPrefix):-1], request
        )

        self.check_ordinary_block(request, blockedIp)

    def test_xml_partner_search(self):
        blockedIp = "127.0.0.1"
        self.block_ip(blockedIp)

        req = urllib.request.Request(
            r"http://xmlsearch.yandex.ru/xmlsearch?text=123&showmecaptcha=yes"
        )
        resp = self.antirobot.send_request(
            Fullreq(req, headers={REAL_IP_HEADER: blockedIp})
        )
        assert resp.getcode() == 200

        xml = ElementTree.fromstring(resp.read().decode())
        assert xml.tag == "yandexsearch"
        xml = xml.find("response")
        assert xml is not None
        xml = xml.find("error")
        assert xml is not None
        assert "code" in xml.attrib.keys()
        assert xml.attrib["code"] == "101"

    def check_ordinary_block(self, request, ip):
        resp = self.antirobot.send_request(
            Fullreq(request, headers={IP_HEADER: ip})
        )
        assert resp.getcode() == 403
        assert len(resp.read()) > 0

    @pytest.mark.parametrize("request_url", [
        r"http://yandex.com.tr/yandsearch?text=123",
        r"http://yandex.com.tr/sitesearch?text=123",
        r"http://yandex.com.tr/favicon.ico",  # non-search request
    ])
    def test_ordinary_block(self, request_url):
        blockedIp = self.block_ip()
        self.check_ordinary_block(request_url, blockedIp)

    def test_no_ban_for_xml_search(self):
        """
        Мы не блокируем непартнёрский XML-поиск
        """
        ip = GenRandomIP()
        resp = self.antirobot.send_request(
            Fullreq(
                "http://xmlsearch.yandex.ru/xmlsearch?text=123",
                headers={IP_HEADER: ip},
            )
        )
        AssertNotRobotResponse(resp)

        self.block_ip(ip)

        # block directly by IP is useless for XML requests
        resp = self.antirobot.send_request(
            Fullreq(
                "http://xmlsearch.yandex.ru/xmlsearch?text=123",
                headers={IP_HEADER: ip},
            )
        )
        AssertNotRobotResponse(resp)

    @pytest.mark.parametrize("reqFunc, contentType", [
        (
            lambda ip: Fullreq(
                r"http://yandex.ru/yandsearch?text=123",
                headers={IP_HEADER: ip},
            ),
            "text/html",
        ),
        (
            lambda ip: Fullreq(
                r"http://xmlsearch.yandex.ru/xmlsearch?text=123&showmecaptcha=yes",
                headers={REAL_IP_HEADER: ip},
            ),
            "text/xml",
        ),
        (
            lambda ip: Fullreq(
                r"http://yandex.ru/yandsearch?text=123&ajax=1&format=json",
                headers={IP_HEADER: ip},
            ),
            "application/json",
        ),
        (
            lambda ip: Fullreq(
                r"http://yandex.ru/yandsearch?text=123&ajax=1&callback=some_callback",
                headers={IP_HEADER: ip},
            ),
            "application/javascript",
        ),
    ])
    def test_content_type(self, reqFunc, contentType):
        blockedIp = self.block_ip()
        resp = self.antirobot.send_request(reqFunc(blockedIp))
        assert resp.headers.get("Content-Type") == contentType

    def test_stat_by_service(self):
        blockedIp = self.block_ip()

        self.antirobot.send_request(fullreq_for_service("avia", blockedIp))
        self.antirobot.send_request(fullreq_for_service("avia", blockedIp))
        self.antirobot.send_request(fullreq_for_service("market", blockedIp))
        self.antirobot.send_request(fullreq_for_service("invest", blockedIp))

        assert self.antirobot.query_metric("block_responses_total_deee", service_type="avia") == 2
        assert self.antirobot.query_metric("block_responses_total_deee", service_type="market") == 1
        assert self.antirobot.query_metric("block_responses_total_deee", service_type="invest") == 1

    def test_blocked_yandex_ips(self):
        blockedIp = self.block_ip(YANDEX_IP)

        self.antirobot.send_request(fullreq_for_service("avia", blockedIp))
        self.antirobot.send_request(fullreq_for_service("avia", blockedIp))
        self.antirobot.send_request(fullreq_for_service("market", blockedIp))
        self.antirobot.send_request(fullreq_for_service("invest", blockedIp))

        assert self.antirobot.query_metric("block_responses_from_yandex_deee", service_type="avia") == 2
        assert self.antirobot.query_metric("block_responses_from_yandex_deee", service_type="market") == 1
        assert self.antirobot.query_metric("block_responses_from_yandex_deee", service_type="invest") == 1

    def test_blocked_from_whitelist(self):
        blockedIp = self.block_ip(WHITELIST_IP)

        self.antirobot.send_request(fullreq_for_service("avia", blockedIp))
        self.antirobot.send_request(fullreq_for_service("avia", blockedIp))
        self.antirobot.send_request(fullreq_for_service("market", blockedIp))
        self.antirobot.send_request(fullreq_for_service("invest", blockedIp))

        assert self.antirobot.query_metric("block_responses_from_whitelist_deee", service_type="avia") == 2
        assert self.antirobot.query_metric("block_responses_from_whitelist_deee", service_type="market") == 1
        assert self.antirobot.query_metric("block_responses_from_whitelist_deee", service_type="invest") == 1

    def test_block_with_422_code(self):
        blockedIp = self.block_ip()
        response = self.antirobot.send_request(fullreq_for_service("invest", blockedIp))
        assert response.getcode() == 422

    def test_verify_callback_correctness_for_json(self):
        blockedIp = self.block_ip()

        resp = self.antirobot.send_request(
            Fullreq(
                r"http://yandex.ru/yandsearch?text=123&format=json&ajax=1&callback=abC_190",
                headers={IP_HEADER: blockedIp},
            )
        )
        assert resp.headers.get("Content-Type") == "application/javascript"
        assert resp.getcode() == 200

        resp = self.antirobot.send_request(
            Fullreq(
                r"http://yandex.ru/yandsearch?text=123&format=json&ajax=1&callback=!%.",
                headers={IP_HEADER: blockedIp},
            )
        )
        assert resp.getcode() == 400

        bannedIp = GenRandomIP()
        self.antirobot.ban(bannedIp)

        resp = self.antirobot.send_request(
            Fullreq(
                r"http://yandex.ru/yandsearch?text=123&format=json&ajax=1&callback=sc0",
                headers={IP_HEADER: bannedIp},
            )
        )
        assert resp.headers.get("Content-Type") == "application/javascript"
        assert resp.getcode() == 200

        resp = self.antirobot.send_request(
            Fullreq(
                r"http://yandex.ru/yandsearch?text=123&format=json&ajax=1&callback=!!!",
                headers={IP_HEADER: bannedIp},
            )
        )
        assert resp.getcode() == 400

    def test_blocked_lcookies(self):
        blockedIp = self.block_ip()

        blocked_lcookies = self.antirobot.query_metric("block_responses_with_Lcookie_deee")
        unique_blocked_lcookies = self.antirobot.query_metric("block_responses_with_unique_Lcookies_ahhh")

        self.antirobot.send_fullreq(
            "http://yandex.ru/search?query=banned_web",
            headers={
                IP_HEADER: blockedIp,
                "Cookie": ("L=%s" % VALID_L_COOKIE),
            },
        )

        assert self.antirobot.query_metric("block_responses_with_Lcookie_deee") == blocked_lcookies + 1
        assert self.antirobot.query_metric("block_responses_with_unique_Lcookies_ahhh") == unique_blocked_lcookies + 1

        self.antirobot.send_fullreq(
            "http://yandex.ru/search?query=banned_web",
            headers={
                IP_HEADER: blockedIp,
                "Cookie": ("L=%s" % VALID_L_COOKIE),
            },
        )

        assert self.antirobot.query_metric("block_responses_with_Lcookie_deee") == blocked_lcookies + 2
        assert self.antirobot.query_metric("block_responses_with_unique_Lcookies_ahhh") == unique_blocked_lcookies + 1

        time.sleep(REMOVE_EXPIRED_PERIOD + AMNESTY_INTERVAL_SHORT + SYNC_LAG)

        assert self.antirobot.query_metric("block_responses_with_unique_Lcookies_ahhh") == 0
