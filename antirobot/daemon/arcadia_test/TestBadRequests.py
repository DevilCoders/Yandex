from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    GenRandomIP,
    Fullreq,
    asserts,
)


class TestBadRequests(AntirobotTestSuite):
    options = {
    }

    def test_invalid_content(self):
        ip = GenRandomIP()
        self.antirobot.block(ip)

        req = Fullreq(req="http://yandex.ru/search?text=Bump", headers={
            "X-Forwarded-For-Y": ip,
            "Content-Encoding": "br",
        }, content="Hacked")
        resp = self.send_request(req)
        asserts.AssertBlocked(resp)
