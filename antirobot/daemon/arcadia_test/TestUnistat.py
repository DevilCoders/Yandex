import pytest

from antirobot.daemon.arcadia_test.util import GenRandomIP
from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite


class TestUnistat(AntirobotTestSuite):
    @pytest.mark.parametrize('url, service', [
        ("http://yandex.ru/search", "web"),
        ("http://images.yandex.ru/search", "img"),
    ])
    def test_handle_time(self, url, service):
        ip = GenRandomIP()
        metric_before = self.antirobot.get_metric(f"service_type={service};handle_time_10s_deee")
        self.send_fullreq(url, headers={"X-Forwarded-For-Y": ip})
        metric_after = self.antirobot.get_metric(f"service_type={service};handle_time_10s_deee")
        assert metric_before + 1 == metric_after
