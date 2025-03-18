import time

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    Fullreq
)

REQUEST_WEB = Fullreq(
    "http://yandex.ru/search?text=i_am_not_robot",
    headers={"X-Forwarded-For-Y": '22.13.42.4'},
)

REQUEST_COLLECTIONS = Fullreq(
    "http://yandex.ru/collections?text=i_am_not_robot",
    headers={"X-Forwarded-For-Y": '22.13.42.4'},
)


class TestExperimentFormulas(AntirobotTestSuite):
    def test_experiment_config(self):
        self.antirobot.send_request(REQUEST_WEB)
        time.sleep(1)
        last_daemon_log = self.unified_agent.get_last_event_in_daemon_logs()
        assert 'cacher_catboost_v3.info' in last_daemon_log['cacher_exp_formulas']
        assert 'catboostWebExp2.info' in last_daemon_log['exp_formulas']

        self.antirobot.send_request(REQUEST_COLLECTIONS)
        time.sleep(1)
        last_daemon_log = self.unified_agent.get_last_event_in_daemon_logs()
        assert 'cacher_exp_formulas' not in last_daemon_log.keys()
        assert 'exp_formulas' not in last_daemon_log.keys()
