import base64
import calendar
import datetime
from pathlib import Path
import time
import http.cookies
import hmac
import hashlib

from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import jwt
import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite, TEST_DATA_ROOT
from antirobot.daemon.arcadia_test.util import Fullreq, GenRandomIP
from antirobot.daemon.arcadia_test.util.asserts import AssertBlocked
from antirobot.idl import antirobot_cookie_pb2


IP_HEADER = "X-Forwarded-For-Y"
IS_EU_HEADER = "X-Yandex-EU-Request"
IS_BOT_HEADER = "X-Antirobot-Is-Crawler"
REGION_HEADER = "X-Antirobot-Region-Id"
DEGRADATION_HEADER = "X-Yandex-Antirobot-Degradation"
BAN_SOURCE_IP = "X-Antirobot-Ban-Source-Ip"
EU_IP = "5.198.255.112"
MOSCOW_IP = "87.250.250.242"
CHINA_IP = "59.151.106.224"
YANDEX_TRUST_VALID_KEY = "189C09EB0B6BB890403547C7E87A0D4BCC67AA0B1B6C00E2806F7ECCAAE1156C"


def get_last_visits(key, cookie):
    decoded_cookie = base64.b64decode(cookie)
    decrypted_cookie = AESGCM(key).decrypt(decoded_cookie[:12], decoded_cookie[12:], None)
    parsed_cookie = antirobot_cookie_pb2.TAntirobotCookie()
    parsed_cookie.ParseFromString(decrypted_cookie)

    vec = parsed_cookie.LastVisits[4:]
    return {vec[2 * i]: vec[2 * i + 1] for i in range(len(vec) // 2) if vec[2 * i] in [42, 66]}


def get_antirobot_set_cookie(response):
    cookie = response.headers.get("X-Antirobot-Set-Cookie")

    if cookie is None:
        return None

    yasc_cookie = http.cookies.BaseCookie(cookie).get('_yasc')
    assert yasc_cookie.get('domain') == '.yandex.ru'
    assert yasc_cookie.get('path') == '/'
    assert yasc_cookie.get('secure')

    return yasc_cookie.value


def load_jws_key(path):
    with open(path) as file:
        content = file.read()

    tokens = content.strip().split(":")

    if len(tokens) != 3 or tokens[1] != "HS256":
        raise Exception(f"Invalid JWS token: {path}")

    return tokens[0], bytes.fromhex(tokens[2])


class TestBalancerHeaders(AntirobotTestSuite):
    bot_ips_file = Path.cwd() / "search_engine_bots"
    bot_ip = "192.168.1.1"
    regular_ip = "192.168.1.2"

    options = {
        "BalancerJwsKeyPath": TEST_DATA_ROOT / "data" / "balancer_jws_key",
        "DisableBansByFactors": 1,
        "InitialChinaRedirectEnabled": False,
        "SearchBotsFile": bot_ips_file,
    }

    @classmethod
    def setup_class(cls):
        with open(cls.bot_ips_file, "wt") as f:
            t = int((time.time() + 86400)*1000000)
            print(f"{cls.bot_ip}\t{t}\tGoogleBot", file=f)

        super().setup_class()

    @classmethod
    def setup_subclass(cls):
        with open(TEST_DATA_ROOT / "data" / "yasc_key") as yasc_key_file:
            cls.yasc_key = bytes.fromhex(yasc_key_file.read().strip())

    global_config = {
        "rules": [
            {
                "id": 0,
                "cbb": [],
                "yql": ["another_factor == 0"],
            }
        ],
        "last_visits": [
            {
                "id": 42,
                "name": "really_cool_factor",
                "rule": "cgi=/.*really_cool_factor.*/",
            },
            {
                "id": 66,
                "name": "another_factor",
                "rule": "cgi=/.*another_factor.*/",
            }
        ]
    }

    @pytest.mark.parametrize("header, addr, value", [
        (IS_EU_HEADER, MOSCOW_IP, "0"),
        (IS_EU_HEADER, EU_IP, "1"),
        (IS_BOT_HEADER, regular_ip, None),
        (IS_BOT_HEADER, bot_ip, "GoogleBot"),
        (REGION_HEADER, MOSCOW_IP, "225"),
        (REGION_HEADER, EU_IP, "123"),
        (REGION_HEADER, CHINA_IP, "134"),
        (DEGRADATION_HEADER, regular_ip, "0"),
    ])
    def test_balancer_header(self, header, addr, value):
        response = self.antirobot.send_request(Fullreq(
            "http://yandex.ru/search?text=hello",
            headers={IP_HEADER: addr},
        ))
        assert response.info()[header] == value

    @pytest.mark.parametrize("addr, value", [
        (regular_ip, None),
        (bot_ip, "GoogleBot"),
    ])
    def test_ip_list(self, addr, value):
        self.antirobot.send_request(Fullreq(
            "http://yandex.ru/search?text=hello",
            headers={IP_HEADER: addr},
        ))
        event = self.unified_agent.get_last_event_in_daemon_logs(addr)
        ip_list = event.get("ip_list", "").split(",")
        if value:
            assert "crawler:" + value in ip_list

    @pytest.mark.parametrize("header, addr, value", [
        (IS_EU_HEADER, MOSCOW_IP, "0"),
        (IS_EU_HEADER, EU_IP, "1"),
        (IS_BOT_HEADER, regular_ip, None),
        (IS_BOT_HEADER, bot_ip, None),
        (REGION_HEADER, MOSCOW_IP, None),
        (REGION_HEADER, EU_IP, None),
        (DEGRADATION_HEADER, regular_ip, None),
        (BAN_SOURCE_IP, regular_ip, None),
    ])
    def test_not_passed_to_user(self, header, addr, value):
        # проверяем, что заголовок проходит в т.ч. юзеру
        self.antirobot.block(addr)

        response = self.antirobot.send_request(Fullreq(
            "http://yandex.ru/search?text=hello",
            headers={IP_HEADER: addr},
        ))
        AssertBlocked(response)
        assert response.info().get(header) == value

    def test_yasc(self):
        ip = GenRandomIP()

        response1 = self.antirobot.ping_search(ip=ip, query="really_cool_factor")
        ar_cookie1 = get_antirobot_set_cookie(response1)
        assert get_last_visits(self.yasc_key, ar_cookie1) == {42: 1}

        response2 = self.antirobot.ping_search(
            ip=ip,
            query="another_factor",
            headers={"Cookie": f"_yasc={ar_cookie1}"},
        )

        last_visits2 = get_last_visits(self.yasc_key, get_antirobot_set_cookie(response2))
        assert last_visits2 in ({42: 1, 66: 1}, {42: 2, 66: 1})

    def test_yasc_rule(self):
        ip = GenRandomIP()

        def step(query, input_cookie, last_visits, rules):
            headers = {}

            if input_cookie is not None:
                headers["Cookie"] = f"_yasc={input_cookie}"

            response = self.antirobot.ping_search(ip=ip, query=query, headers=headers)
            ar_cookie = get_antirobot_set_cookie(response) or input_cookie
            assert get_last_visits(self.yasc_key, ar_cookie) == last_visits

            event = self.unified_agent.get_last_event_in_daemon_logs(ip)
            assert event.get("rules", []) == rules

            return ar_cookie

        ar_cookie = step("foo", None, {}, ["0"])
        ar_cookie = step("another_factor", ar_cookie, {66: 1}, [])
        step("bar", ar_cookie, {66: 1}, [])

    def test_yasc_clean(self):
        ip = GenRandomIP()

        response1 = self.antirobot.ping_search(ip=ip, query="another_factor")
        response2 = self.antirobot.ping_search(
            ip=ip,
            query="another_factor",
            headers={"Cookie": response1.headers.get("X-Antirobot-Set-Cookie")},
        )
        assert "X-Antirobot-Set-Cookie" not in response2.headers

    def test_hodors(self):
        ip = GenRandomIP()
        response = self.send_fullreq(
            "http://yandex.ru/search?text=hello",
            headers={"X-forwarded-for-y": ip, "User-agent": "Gorilla firefox", "Accept-language": "ru"},
        )
        assert response.info()["X-Antirobot-Hodor"] == "be-cv-an"
        assert len(response.info()["X-Antirobot-Hodor-Hash"]) != 0

    @pytest.mark.parametrize("mode, expired", [
        ("INVALID", False),
        ("VALID", True),
        ("VALID", False),
        ("DEFAULT", True),
        ("DEFAULT", False),
        ("SUSP", True),
        ("SUSP", False),
    ])
    def test_jws_info(self, mode, expired):
        ip = GenRandomIP()

        now = calendar.timegm(datetime.datetime.utcnow().utctimetuple())
        exp_diff = -3600 if expired else 3600
        claims = {"expires_at_ms": (now + exp_diff) * 1000}
        headers = {}

        if mode == "INVALID":
            key = base64.b64decode("z5759hvmZv7poXa9/SSaq1mh7jVZxQROudlIr35Hdhg=")
        elif mode in ("VALID", "SUSP"):
            headers["kid"], key = load_jws_key(TEST_DATA_ROOT / "data" / "narwhal_jws_key")
            claims["device_integrity"] = mode == "VALID"
        elif mode == "DEFAULT":
            headers["kid"], key = load_jws_key(TEST_DATA_ROOT / "data" / "balancer_jws_key")

        if expired:
            mode += "_EXPIRED"

        token = jwt.encode(claims, key, algorithm="HS256", headers=headers)
        response = self.antirobot.ping_search(ip=ip, headers={"X-Yandex-Jws": token})
        assert response.headers.get("X-Antirobot-Jws-Info") == mode

    @pytest.mark.parametrize("mode, expired", [
        ("INVALID", False),
        ("VALID", False),
        ("VALID", True),
        ("SUSP", False),
        ("SUSP", True),
    ])
    def test_market_jws_info(self, mode, expired):
        ip = GenRandomIP()

        now = calendar.timegm(datetime.datetime.utcnow().utctimetuple())
        exp_diff = -3600 if expired else 3600
        claims = {"exp": now + exp_diff}
        headers = {}

        if mode == "INVALID":
            key = base64.b64decode("z5759hvmZv7poXa9/SSaq1mh7jVZxQROudlIr35Hdhg=")
        elif mode in ("VALID", "SUSP"):
            with open(TEST_DATA_ROOT / "data" / "market_jws_key") as key_file:
                key = base64.b64decode(key_file.read().strip())
            claims["basicIntegrity"] = mode == "VALID"
            claims["ctsProfileMatch"] = mode == "VALID"

        if expired:
            mode += "_EXPIRED"

        token = jwt.encode(claims, key, algorithm="HS256", headers=headers)
        response = self.antirobot.ping_search(ip=ip, headers={"X-Jws": token})
        assert response.headers.get("X-Antirobot-Jws-Info") == mode

    def yandex_trust_sign(self, msg, key):
        return hmac.new(bytes.fromhex(key), msg=msg.encode(), digestmod=hashlib.sha256).hexdigest().upper()

    @pytest.mark.parametrize("expected_info, data, digest", [
        ("VALID",         "key_id=0;time=13286475876140248;attestation=;",  "digest={valid};"),
        ("INVALID",       "key_id=0;time=13286475876140248;attestation=;",  "digest=1171D1E821AC93899F6C32630B9D66CB7F6EC18D85F55C8A69F3A19673846FE2;"),
        ("INVALID",       "key_id=0;time=13286475876140248;attestation=;",  "digest={valid}XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX;"),
        ("VALID_EXPIRED", "key_id=0;time=11111111111111111;attestation=;",  "digest={valid};"),
        ("VALID_EXPIRED", "key_id=0;time=13186475876000000;attestation=;",  "digest={valid};"),
        ("INVALID",       "key_id=0;time=13286475876140248;attestation=;",  ""),
        ("INVALID",       "key_id=0;t_ime=13286475876140248;attestation=;", "digest={valid};"),
        ("VALID_EXPIRED", "key_id=0;time=XX;attestation=;",                 "digest={valid};"),
        ("INVALID",       "",                                               ""),
        (None,            None,                                             None),
    ])
    def test_yandex_trust_info_valid(self, expected_info, data, digest):
        ip = GenRandomIP()

        headers = {
            "X-Start-Time": "1642002276000000",
        }
        token = None
        if data is not None:
            token = data + digest.format(valid=self.yandex_trust_sign(data, YANDEX_TRUST_VALID_KEY))

        if token is not None:
            headers["Yandex-Trust"] = token
        response = self.antirobot.ping_search(ip=ip, headers=headers, query=f"query_{ip}")

        assert response.headers.get("X-Antirobot-Yandex-Trust-Info") == expected_info
        daemon_log = self.unified_agent.wait_log_line_with_query(f".*query_{ip}.*")
        assert daemon_log["yandex_trust_state"] == (expected_info or "INVALID")
