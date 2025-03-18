import yatest

import base64
import hashlib
import itertools
import json
import os
import time
import urllib.parse

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    GenRandomIP,
    captcha_page,
    spravka,
)
from antirobot.daemon.arcadia_test.util.asserts import (
    AssertCaptchaRedirect,
)


def antirobot_mac(s, key):
    return hashlib.md5(s + key).hexdigest()


def xor_encrypt(s, key):
    return bytes(a ^ b for a, b in zip(s, itertools.cycle(key)))


class CaptchaSignatureData:
    __signature_key = None

    def __init__(self, suite, ip):
        self.encryption_key = os.urandom(32)
        self.encoded_encryption_key = base64.b64encode(self.encryption_key).decode()

        timestamp = int(time.time())
        rnd = "4"  # https://xkcd.com/221
        signature_data = "_".join((
            self.encoded_encryption_key,
            "1",
            str(timestamp),
            ip,
            rnd,
        )).encode()

        signature = antirobot_mac(signature_data, self.__get_signature_key(suite).encode())
        self.signature = f"1_{timestamp}_{rnd}_{signature}"

    def encrypt_encode(self, cleartext):
        ciphertext = xor_encrypt(cleartext.encode(), self.encryption_key)
        return base64.b64encode(ciphertext).decode()

    def generate_post_body(self, js_print):
        if not isinstance(js_print, str):
            js_print = json.dumps(js_print)

        return urllib.parse.urlencode({
            "d": self.encoded_encryption_key,
            "k": self.signature,
            "rdata": self.encrypt_encode(js_print),
        })

    @classmethod
    def __get_signature_key(cls, suite):
        if cls.__signature_key is None:
            with open(suite.keys_path()) as file:
                cls.__signature_key = next(file).rstrip("\n")

        return cls.__signature_key


class TestVerochka(AntirobotTestSuite):
    @classmethod
    def setup_class(cls):
        cls.js_print_short_to_long = {}
        with open(yatest.common.source_path("antirobot/captcha/generated/js_print_mapping.json")) as inp:
            mapping = json.load(inp)
            for long_name, short_name in mapping.items():
                cls.js_print_short_to_long[short_name] = long_name

        super().setup_class()

    def setup_method(self, method):
        super().setup_method(method)
        self.unified_agent.pop_event_logs()

    def remap_js_print(self, js_print):
        return {self.js_print_short_to_long[k]: v for k, v in js_print.items()}

    def test_check_verochka_stats(self):
        ip = GenRandomIP()
        bad_ip = GenRandomIP()
        captcha = captcha_page.HtmlCaptchaPage(self, "yandex.ru")

        invalid_verochka_requests = self.antirobot.get_metric("invalid_verochka_requests_deee")

        captcha_redirect = spravka.DoCheckCaptcha(self, ip, captcha, rep="123", content="")
        AssertCaptchaRedirect(captcha_redirect)
        invalid_verochka_requests += 1
        assert self.antirobot.get_metric("invalid_verochka_requests_deee") == invalid_verochka_requests

        signature_data = CaptchaSignatureData(self, ip)

        content = signature_data.generate_post_body({})
        captcha_redirect = spravka.DoCheckCaptcha(self, ip, captcha, rep="123", content=content)
        AssertCaptchaRedirect(captcha_redirect)
        assert self.antirobot.get_metric("invalid_verochka_requests_deee") == invalid_verochka_requests

        captcha_redirect = spravka.DoCheckCaptcha(self, bad_ip, captcha, rep="123", content=content)
        AssertCaptchaRedirect(captcha_redirect)
        invalid_verochka_requests += 1
        assert self.antirobot.get_metric("invalid_verochka_requests_deee") == invalid_verochka_requests

        content = signature_data.generate_post_body("{invalid_json")
        captcha_redirect = spravka.DoCheckCaptcha(self, ip, captcha, rep="123", content=content)
        AssertCaptchaRedirect(captcha_redirect)
        invalid_verochka_requests += 1
        assert self.antirobot.get_metric("invalid_verochka_requests_deee") == invalid_verochka_requests

    # def test_check_verochka_input_logged_invalid(self):
    #     ip = GenRandomIP()
    #     captcha = captcha_page.HtmlCaptchaPage(self, "yandex.ru")
    #     js_print = '{invalid_json'
    #     request_content = f"verochka-data={urllib.parse.quote(js_print)}"
    #     captcha_redirect = spravka.DoCheckCaptcha(self, ip, captcha, rep="123", content=request_content)
    #     AssertCaptchaRedirect(captcha_redirect)

    #     log_entry = self.antirobot.get_last_event_in_verochka_logs()
    #     assert json.loads(log_entry["js_print"]) == dict(self.remap_js_print({
    #         "a1": "",
    #         "a2": "",
    #         "a3": "",
    #         "a4": "",
    #         "a5": "",
    #         "a6": 0,
    #         "a7": False,
    #         "a8": False,
    #         "a9": False,
    #         "b1": False,
    #         "b2": False,
    #         "b3": False,
    #         "b4": False,
    #         "b5": False,
    #         "b6": False,
    #         "b7": False,
    #         "b8": False,
    #         "b9": False,
    #         "c1": 0,
    #         "c2": "",
    #         "c3": "",
    #         "c4": False,
    #         "c5": False,
    #         "c6": 0,
    #         "c7": False,
    #         "c8": 0,
    #         "c9": "",
    #         "d1": "",
    #         "d2": 0,
    #         "d3": False,
    #         "d4": False,
    #         "d5": False,
    #         "d7": 0,
    #         "d8": "",
    #         "d9": False,
    #         "e1": False,
    #         "e2": False,
    #         "e3": False,
    #         "e4": False,
    #         "e5": False,
    #         "e6": False,
    #         "e7": False,
    #         "e8": False,
    #         "e9": False,
    #         "f1": False,
    #         "f2": False,
    #         "f3": False,
    #         "f4": False,
    #         "f5": False,
    #         "f6": False,
    #         "f7": False,
    #         "f8": False,
    #         "f9": False,
    #         "g1": False,
    #         "g2": False,
    #         "g3": False,
    #         "g4": False,
    #         "g5": False,
    #         "g6": False,
    #         "g7": False,
    #         "g8": False,
    #         "g9": False,
    #         "h1": False,
    #         "h2": False,
    #         "h3": False,
    #         "h4": False,
    #         "h5": False,
    #         "h6": False,
    #         "v": "",
    #         "z1": "",
    #         "z2": "",
    #     }), valid_json=False, has_data=True)
    #     assert not log_entry["is_valid"]
    #     assert log_entry["user_ip"] == ip
    #     assert log_entry["event_unixtime"]
    #     assert log_entry["ident_type"]
    #     assert log_entry["request_content"] == request_content

    def test_check_verochka_input_logged_valid(self):
        ip = GenRandomIP()
        captcha = captcha_page.HtmlCaptchaPage(self, "yandex.ru")
        js_print = {
            "a1": "ru",
            "a2": "Google Inc.",
            "a3": "20030107",
            "a4": "ru.en",
            "a5": "Gecko",
            "a6": 4,
            "a7": False,
            "a8": False,
            "a9": False,
            "b1": True,
            "b2": False,
            "b3": False,
            "b4": False,
            "b5": False,
            "b6": False,
            "b7": False,
            "b8": False,
            "b9": False,
            "c1": 33,
            "c2": "954x1680",
            "c3": "1680x1050",
            "c4": True,
            "c5": True,
            "c6": 24,
            "c7": True,
            "c8": 8,
            "c9": "",
            "d1": "",
            "d2": 8,
            "d3": False,
            "d4": True,
            "d5": True,
            "d6": "tz",
            "d7": -180,
            "d8": "0.False.False",
            "d9": True,
            "e1": False,
            "e2": True,
            "e3": True,
            "e4": True,
            "e5": False,
            "e6": True,
            "e7": True,
            "e8": True,
            "e9": True,
            "f1": False,
            "f2": True,
            "f3": False,
            "f4": True,
            "f5": False,
            "f6": False,
            "f7": False,
            "f8": False,
            "f9": True,
            "g1": False,
            "g2": False,
            "g3": True,
            "g4": False,
            "g5": True,
            "g6": True,
            "g7": True,
            "g8": True,
            "g9": False,
            "h1": False,
            "h2": False,
            "h3": False,
            "h4": False,
            "h5": True,
            "h6": False,
            "h7": "audiocontext",
            "h8": True,
            "h9": True,
            "i1": True,
            "i2": True,
            "i3": True,
            "i4": True,
            "i5": True,
            "z3": "z3",
            "z4": "z4",
            "z5": True,
            "z6": "z6",
            "z7": "z7",
            "z8": "z8",
            "z9": "z9",
            "y1": "y1",
            "y2": "y2",
            "y3": "y3",
            "y4": "y4",
            "y5": "y5",
            "y6": "y6",
            "y7": "y7",
            "y8": "y8",
            "y9": "y9",
            "y10": "y10",
            "x1": "x1",
            "x2": "x2",
            "x3": "x3",
            "x4": "x4",
            "x5": "x5",
            "v": "1.1.1",
            "z1": "ddd4548797fea7a1b8f0533709e890ec",
            "z2": "db7196c6a1db31450938d39450b997b7",
        }
        request_content = CaptchaSignatureData(self, ip).generate_post_body(js_print)
        captcha_redirect = spravka.DoCheckCaptcha(self, ip, captcha, rep="123", content=request_content)
        AssertCaptchaRedirect(captcha_redirect)

        log_entry = self.unified_agent.get_last_event_in_verochka_logs()

        assert json.loads(log_entry.Event.JsPrint) == dict(self.remap_js_print(js_print), valid_json=True, has_data=True)
        assert bool(log_entry.Event.IsValid)
        assert log_entry.Event.Header.Addr.decode("utf-8") == ip
        uid_ip = sum(int(x) << (8 * i) for i, x in enumerate(reversed(ip.split("."))))
        assert log_entry.Event.Header.Uid.decode("utf-8") == f"1-{uid_ip}"
        assert log_entry.Event.RequestContent == request_content

    def test_input_logged_if_balancer_cuts_content_length(self):
        self.unified_agent.pop_event_logs()
        ip = GenRandomIP()
        captcha = captcha_page.HtmlCaptchaPage(self, "yandex.ru")
        check_req = captcha.get_check_captcha_url() + "&rep=qqq"
        js_print = {"a1": "lang"}
        request_content = CaptchaSignatureData(self, ip).generate_post_body(js_print)
        request = Fullreq(check_req, headers={'X-Forwarded-For-Y': ip}, content="", method="POST")
        request.Data += b"\r\n" + request_content.encode()
        AssertCaptchaRedirect(self.send_request(request))

        log_entry = self.unified_agent.get_last_event_in_verochka_logs()

        assert bool(log_entry.Event.IsValid)
        assert json.loads(log_entry.Event.JsPrint)["browser_navigator_language"] == "lang"
        assert log_entry.Event.RequestContent == request_content

    def test_tov_major(self):
        resp = self.send_request(Fullreq(r"http://yandex.ru/tmgrdfrendc"))
        assert resp.getcode() == 200
        assert resp.read().decode() == "{}"

    def test_tov_major_script(self):
        resp = self.send_request(Fullreq(r"http://yandex.ru/tmgrdfrend.js"))
        assert resp.getcode() == 200
        assert "/tmgrdfrendc" in resp.read().decode()
