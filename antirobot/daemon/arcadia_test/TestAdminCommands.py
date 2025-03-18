import json
import os
import random
import time
import xml.etree

import pytest

from antirobot.daemon.arcadia_test import util
from antirobot.daemon.arcadia_test.util import asserts
from antirobot.daemon.arcadia_test.util import captcha_page
from antirobot.daemon.arcadia_test.util import req_sender

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite

from antirobot.tools.yasm_stats import lib as yasm_stats_lib


BLOCK_AMNESTY_PERIOD = 1
LONG_TEST_TIMEOUT = 180
CAPTCHA_PAGE_TEST_PARAMS = ("host, doc, service, add_service_header", (
    ("yandex.ru", "/search", "web", False),
    ("auto.ru", "/search", "autoru", True),
    ("kinopoisk.ru", "/search", "kinopoisk", True),
    ("yandex.ru", "/images/search", "img", False),
    ("beru.ru", "/search", "marketblue", True),
))


class TestAdminCommands(AntirobotTestSuite):
    options = {
        "RemoveExpiredPeriod": f"{BLOCK_AMNESTY_PERIOD}s",
        "DaemonLogFormatJson": 1,
    }

    captcha_args = ["--generate", "correct", "--check", "success"]

    @classmethod
    def setup_subclass(cls):
        def request_generator():
            while True:
                yield util.Fullreq(
                    "http://yandex.ru/yandsearch?text=123",
                    headers={
                        "Cookie": "YX_SHOW_CAPTCHA=1",
                        "X-Forwarded-For-Y": util.GenRandomIP(),
                    },
                )

        cls.req_sender = cls.enter(
            req_sender.RequestSender(cls, 2, 0.1, request_generator),
        )

    def test_unistats(self):
        service_config_path = self.antirobot.dump_cfg()["JsonConfFilePath"]
        with open(service_config_path) as service_config_file:
            service_config = json.load(service_config_file)

        paths = yasm_stats_lib.get_counter_ids(service_config)

        # If you need more don't forget to update Golovan's config file!
        # https://bb.yandex-team.ru/projects/SEARCH_INFRA/repos/yasm/browse/CONF/agent.antirobot.conf#22
        assert len(paths) <= 55000

        port = self.antirobot.unistat_port
        resp = self.send_request(r"/unistats", port)
        resp_text = resp.read()

        try:
            stats_dict = dict(json.loads(resp_text))
        except Exception as exc:
            assert False, f"failed to parse JSON ({exc}): {resp_text}"

        actual_signals = set(stats_dict.keys())
        expected_signals = set(paths)
        assert actual_signals == expected_signals

        for p in paths:
            value = stats_dict[p]
            assert isinstance(value, (int, float))

    def test_unistats_check_empty_json(self):
        port = self.antirobot.unistat_port
        resp = self.send_request(r"/unistats?service=img", port)
        assert dict(json.load(resp)) == {}

    @pytest.mark.parametrize("service", (
        "news",
        "rabota",
        "realty",
        "travel",
        "music",
        "img",
        "video",
        "avia",
    ))
    def test_unistats_for_service(self, service):
        if service == "music":
            path = "api/search?text="
        else:
            path = "search?text="

        request = util.Fullreq(
            f"http://yandex.ru/{path}{captcha_page.FORCED_CAPTCHA_REQUEST}",
            headers={"X-Antirobot-Service-Y": service},
        )
        request_count = random.randrange(1, 10)

        for _ in range(request_count):
            asserts.AssertCaptchaRedirect(self.send_request(request))

        for port in [self.antirobot.admin_port, None]:
            redirect_metric = self.antirobot.query_metric(
                "captcha_redirects_deee",
                service_type=service,
            )

            assert redirect_metric == request_count

    def test_block_stats(self):
        for port in [self.antirobot.admin_port, None]:
            resp = self.send_request(r"/admin?action=blockstats", port)
            assert xml.etree.ElementTree.fromstring(resp.read()) is not None

    def test_reload_data(self):
        for port in [self.antirobot.admin_port, None]:
            self.send_request(r"/admin?action=reloaddata", port)

    def test_unknown_action(self):
        for port in [self.antirobot.admin_port, None]:
            resp = self.send_request(r"/admin?action=some_strange_command_that_antirobot_doesnt_know", port)
            assert resp.getcode() == 400

    def test_unknown_cacher_action(self):
        resp = self.send_request(r"/qwe?foo=bar")
        assert resp.getcode() == 404
        assert "<title>404</title>" in resp.read().decode()

    def assert_file_size_stops_changing(self, filename):
        def has_changed():
            original_size = os.path.getsize(filename)
            for _ in range(10):
                if original_size != os.path.getsize(filename):
                    return True
                time.sleep(0.1)
            return False

        asserts.AssertEventuallyTrue(lambda: not has_changed())

    def assert_file_size_is_growing(self, filename):
        original_size = os.path.getsize(filename)

        for _ in range(round(LONG_TEST_TIMEOUT / 0.1)):
            ok = os.path.getsize(filename) > original_size

            if ok:
                break

            time.sleep(0.1)

        assert ok

    def test_generate_spravka(self):
        for port in [self.antirobot.admin_port, None]:
            resp = self.send_request("/admin?action=getspravka&domain=yandex.ru", port)
            asserts.AssertSpravkaValid(self, resp.read().strip(), domain="yandex.ru", key=self.spravka_data_key())

    def test_generate_multiple_spravka(self):
        count = 5
        for port in [self.antirobot.admin_port, None]:
            resp = self.send_request("/admin?action=getspravka&count=%d&domain=yandex.ru" % count, port)
            spravkas = [x.strip() for x in resp.read().decode().strip().split('\n')]

            for spr in spravkas:
                asserts.AssertSpravkaValid(self, spr, domain="yandex.ru", key=self.spravka_data_key())

    def test_amnesty(self):
        for port in [self.antirobot.admin_port, None]:
            ip = util.GenRandomIP()
            self.antirobot.ban(ip)
            time.sleep(2)

            self.send_request("/admin?action=amnesty", port)

            assert not self.antirobot.is_banned(ip)

    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_force_block(self):
        for port in [self.antirobot.admin_port, None]:
            ip = util.GenRandomIP()
            block_duration = 30

            self.send_request("/admin?action=block&ip=%s&duration=%ds" % (ip, block_duration), port)

            assert self.antirobot.is_blocked(ip)
            time.sleep(block_duration + BLOCK_AMNESTY_PERIOD)
            asserts.AssertEventuallyTrue(lambda: not self.antirobot.is_blocked(ip), secondsBetweenCalls=BLOCK_AMNESTY_PERIOD)

    @pytest.mark.parametrize(*CAPTCHA_PAGE_TEST_PARAMS)
    def test_check_unistats_captcha_correct_inputs(self, host, doc, service, add_service_header):
        self.check_captcha_page_stats(
            host, doc, service, add_service_header,
            [
                "captcha_shows_deee",
                "captcha_advanced_shows_deee",
                "captcha_correct_inputs_deee",
                "captcha_incorrect_checkbox_inputs_deee",
                "captcha_image_shows_deee",
            ],
            captcha_page.HtmlCaptchaPage,
        )

    @pytest.mark.parametrize(*CAPTCHA_PAGE_TEST_PARAMS)
    def test_check_unistats_captcha_incorrect_inputs(self, host, doc, service, add_service_header):
        self.reset_captcha(["--generate", "correct", "--check", "fail"])

        self.check_captcha_page_stats(
            host, doc, service, add_service_header,
            [
                "captcha_shows_deee",
                "captcha_advanced_shows_deee",
                "captcha_incorrect_inputs_deee",
                "captcha_incorrect_checkbox_inputs_deee",
                "captcha_image_shows_deee",
            ],
            captcha_page.HtmlCaptchaPage,
        )

    def check_captcha_page_stats(
        self,
        host,
        doc,
        service,
        add_service_header,
        metric_subkeys,
        page_cls,
    ):
        headers = {"X-Antirobot-MayBanFor-Y": 1}
        if add_service_header:
            headers["X-Antirobot-Service-Y"] = service

        old_stats = self.antirobot.get_stats()

        page = page_cls(self, host, doc, headers)

        captcha_page.HtmlCheckCaptcha(
            self, host,
            page.get_key(),
            "zhest",
            page.get_ret_path(),
            headers,
        )

        for image_url in page.get_img_urls():
            response = self.send_fullreq(image_url, headers=headers)
            assert response.getcode() == 302
            response.read()

        stats = self.antirobot.get_stats()

        for subkey in metric_subkeys:
            assert (
                self.antirobot.query_metric(subkey, service_type=service, stats=old_stats) ==
                self.antirobot.query_metric(subkey, service_type=service, stats=stats) - 1
            ), f"key={subkey}"


class TestAdminCommandsCaptchaApi(AntirobotTestSuite):
    options = {
        "AsCaptchaApiService": 1,
    }

    def test_unistats(self):
        service_config_path = self.antirobot.dump_cfg()["JsonConfFilePath"]
        with open(service_config_path) as service_config_file:
            service_config = json.load(service_config_file)

        paths = yasm_stats_lib.get_counter_ids(service_config, True)

        resp = self.send_request(r"/unistats", self.antirobot.unistat_port)
        resp_text = resp.read()

        try:
            stats_dict = dict(json.loads(resp_text))
        except Exception as exc:
            assert False, f"failed to parse JSON ({exc}): {resp_text}"

        actual_signals = set(stats_dict.keys())
        expected_signals = set(paths)
        assert actual_signals == expected_signals

        for p in paths:
            value = stats_dict[p]
            assert isinstance(value, (int, float))

        # If you need more don't forget to update Golovan's config file!
        # https://bb.yandex-team.ru/projects/SEARCH_INFRA/repos/yasm/browse/CONF/agent.smart-captcha.conf#8
        assert len(paths) <= 10000
