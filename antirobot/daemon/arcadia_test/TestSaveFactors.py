import sys

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import GenRandomIP


class TestSaveFactors(AntirobotTestSuite):
    options = {
        "DisableBansByFactors": 1,
        "CacherRandomFactorsProbability": 1.0
    }

    def test_marketapi(self):
        ip = GenRandomIP()
        self.send_fullreq("https://ipa.market.yandex.ru/api/v1/?name=resolveComparison&uuid=f7a3266f4f7e48e68404422bb5d44114",
                          headers={
                              "X-Forwarded-For-Y": ip,
                              "X-Antirobot-Service-Y": "marketfapi_blue",
                              "X-Yandex-Ja3": "769,4-5-47-51-50-10-22-19-9-21-18-3-8-20-17-255,,,"
                          },
                          method='POST')
        daemon_log = self.unified_agent.wait_log_line_with_query(r".*")

        print(daemon_log, file=sys.stderr)
        factors = self.unified_agent.wait_event_logs(["TCacherFactors"])[0].Event
        print(factors, file=sys.stderr)
