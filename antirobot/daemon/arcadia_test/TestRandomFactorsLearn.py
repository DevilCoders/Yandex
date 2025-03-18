from pathlib import Path
import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite

from antirobot.daemon.arcadia_test.util.asserts import (
    AssertEventuallyTrue
)
from antirobot.daemon.arcadia_test.util import (
    GenRandomIP,
    Fullreq,
    IsCaptchaRedirect
)

IP = GenRandomIP()
PRIVILEGED_IP = GenRandomIP()


class TestRandomFactorsLearn(AntirobotTestSuite):
    privileged_ips_file = Path.cwd() / "privileged_ips"
    with open(privileged_ips_file, "wt") as f:
        print(PRIVILEGED_IP, file=f)

    num_old_antirobots = 0
    options = {
        "ProxyCaptchaUrls" : "1",
        "TrainingSetGenSchemes" : "default=random_factors",
        'random_factors_fraction': 1,
        "bans_enabled" : False,   # Чтобы нас не забанило
        "PrivilegedIpsFile" : privileged_ips_file,
    }

    @pytest.mark.parametrize('ip_name', ['ip', 'privileged_ip'])
    def test_all_events_are_logged(self, ip_name):
        if ip_name == 'ip':
            ip = IP
        elif ip_name == 'privileged_ip':
            ip = PRIVILEGED_IP

        headers = {
            "X-Forwarded-For-Y": ip,
            "X-Antirobot-Service-Y": "marketblue",
            "X-Antirobot-MayBanFor-Y": 1,
        }

        resp = self.antirobot.send_request(Fullreq("http://beru.ru/search?cvredirect=2&text=123", headers=headers))
        assert not IsCaptchaRedirect(resp)

        AssertEventuallyTrue(lambda: self.antirobot.queues_are_empty(), secondsBetweenCalls=0.1)

        events_with_antirobot_factors = self.unified_agent.get_event_logs(['TAntirobotFactors'])
        events_with_request_data = self.unified_agent.get_event_logs(['TRequestData'])
        self.unified_agent.update_evlog_time()

        notFoundEvents = 0
        if len(events_with_antirobot_factors) == 0:
            notFoundEvents += 1
        if len(events_with_request_data) == 0:
            notFoundEvents += 1

        num_events = 0 if ip == IP else 2
        assert notFoundEvents == num_events
