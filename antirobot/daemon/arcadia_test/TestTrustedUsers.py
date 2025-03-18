import time

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    GenRandomIP,
    Fullreq,
    TRUSTED_I_COOKIE,
    ICOOKIE_HEADER,
    IP_HEADER,
)

REGULAR_SEARCH = "http://yandex.ru/search/?text=cats"


class TestDegradationHeader(AntirobotTestSuite):
    options = {
        "DisableBansByFactors": 1,
    }

    @classmethod
    def setup_class(cls):
        super().setup_class()

    def get_metric(self):
        return self.antirobot.query_metric(
            "requests_trusted_users_deee",
        )

    def test_trusted_user(self):
        ip = GenRandomIP()

        request = Fullreq(
            REGULAR_SEARCH,
            headers={
                IP_HEADER: ip,
                ICOOKIE_HEADER: TRUSTED_I_COOKIE,
                'Cookie': f'yandexuid={TRUSTED_I_COOKIE};'
            }
        )

        metric_cnt = self.get_metric()

        self.send_request(request)
        time.sleep(1)

        assert self.get_metric() == metric_cnt + 1
