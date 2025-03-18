import time

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import asserts
from antirobot.daemon.arcadia_test.util import GenRandomIP, Fullreq


class TestMarkPassedRobotRequests(AntirobotTestSuite):
    num_antirobots = 0

    def send_requests(self, daemon, requestCount, ip):
        for _ in range(requestCount):
            daemon.send_request(Fullreq("http://yandex.ru/search?text=123", headers={
                "X-Forwarded-For-Y" : ip,
            }))

    def test_passed_robot_requests(self):
        requestCount = 100

        with self.start_antirobots({}, num_antirobots=2, mute=self.mute) as antirobots:
            self.send_requests(antirobots[0], requestCount, GenRandomIP())

            for d in antirobots:
                d.send_request("/admin?action=shutdown")
                asserts.AssertEventuallyTrue(
                    lambda: not d.is_alive(),
                    secondsBetweenCalls=0.25,
                )

            PASSED_VALUE = 2

            num_passed = 0
            num_others = 0

            time.sleep(3)
            events = self.unified_agent.pop_daemon_logs()

            for event in events:
                if event['missed'] == PASSED_VALUE:
                    num_passed += 1
                else:
                    num_others += 1

            assert num_passed > 0
            assert num_others > 0
