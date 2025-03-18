import time
from pathlib import Path

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import Fullreq
from antirobot.daemon.arcadia_test.util import asserts

WATCHER_PERIOD = 0.1

REQUEST_SEARCH = Fullreq(
    "http://yandex.ru/search/yandsearch?text=test_req&param1=buble-goom",
    headers={
        "X-Forwarded-For-Y": "1.2.3.1",
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) testo/2.10.289 Version/12.02",
        "Host": "yandex.ru",
        "Referer": "http://www.yandex.ru/referer",
    },
)


REQUEST_KINOPOISK = Fullreq(
    "https://kinopoisk.ru/search?text=rams",
    headers={
        "X-Forwarded-For-Y": "1.2.3.1",
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) testo/2.10.289 Version/12.02",
        "Host": "kinopoisk.ru",
        "Referer": "http://www.yandex.ru/referer",
    },
)


class TestServerError(AntirobotTestSuite):
    Controls = Path.cwd() / "controls"

    options = {
        "HandleServerErrorEnablePath": Controls / "500_enable",
        "HandleServerErrorDisableServicePath": Controls / "500_disable_service",
        "HandleWatcherPollInterval": WATCHER_PERIOD,
    }

    @classmethod
    def setup_class(cls):
        cls.Controls.mkdir(exist_ok=True)
        super().setup_class()

    def setup_method(self, method):
        super().setup_method(method)

        for opt_name in ["HandleServerErrorEnablePath", "HandleServerErrorDisableServicePath"]:
            with open(self.options[opt_name], "w") as f:
                f.write("")

        time.sleep(WATCHER_PERIOD + 0.1)

    def write_its(self, opt_name, value):
        with open(self.options[opt_name], "w") as f:
            f.write(value)

        time.sleep(WATCHER_PERIOD + 0.1)

    def get_metric(self, service):
        stats = self.antirobot.get_stats()
        key = f"service_type={service};server_error_ahhh"
        return stats.get(key)

    def test_start_server_error(self):
        resp = self.antirobot.send_request(REQUEST_SEARCH)
        asserts.AssertNotServerError(resp)

        prev_value = self.get_metric("web")
        self.write_its("HandleServerErrorEnablePath", "enable")
        resp = self.antirobot.send_request(REQUEST_SEARCH)
        asserts.AssertServerError(resp)
        value = self.get_metric("web")
        assert prev_value + 1 == value
        prev_value = value

        self.write_its("HandleServerErrorEnablePath", "")
        resp = self.antirobot.send_request(REQUEST_SEARCH)
        asserts.AssertNotServerError(resp)
        value = self.get_metric("web")
        assert prev_value == value

    def test_disable_one_service(self):
        prev_value_web = self.get_metric("web")
        prev_value_kp = self.get_metric("kinopoisk")
        resp = self.antirobot.send_request(REQUEST_SEARCH)
        asserts.AssertNotServerError(resp)
        resp = self.antirobot.send_request(REQUEST_KINOPOISK)
        asserts.AssertNotServerError(resp)
        value_web = self.get_metric("web")
        value_kp = self.get_metric("kinopoisk")
        assert prev_value_web == value_web
        assert prev_value_kp == value_kp

        self.write_its("HandleServerErrorEnablePath", "enable")
        resp = self.antirobot.send_request(REQUEST_SEARCH)
        asserts.AssertServerError(resp)
        resp = self.antirobot.send_request(REQUEST_KINOPOISK)
        asserts.AssertServerError(resp)
        value_web = self.get_metric("web")
        value_kp = self.get_metric("kinopoisk")
        assert prev_value_web + 1 == value_web
        assert prev_value_kp + 1 == value_kp
        prev_value_web = value_web
        prev_value_kp = value_kp

        self.write_its("HandleServerErrorDisableServicePath", "web")
        resp = self.antirobot.send_request(REQUEST_SEARCH)
        asserts.AssertNotServerError(resp)
        resp = self.antirobot.send_request(REQUEST_KINOPOISK)
        asserts.AssertServerError(resp)
        value_web = self.get_metric("web")
        value_kp = self.get_metric("kinopoisk")
        assert prev_value_web == value_web
        assert prev_value_kp + 1 == value_kp
