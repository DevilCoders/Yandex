from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util.asserts import (
    AssertEventuallyTrue,
    AssertCaptchaRedirect,
    AssertNotRobotResponse,
)
from antirobot.daemon.arcadia_test.util import (
    GenRandomIP,
    Fullreq,
)
from antirobot.daemon.arcadia_test.util.spravka import GetSpravkaForAddr


class TestMatrixnetThresholdsBase(AntirobotTestSuite):
    def send_web_request(self, ip):
        return self.send_request(Fullreq("http://yandex.ru/yandsearch?text=some", headers={'X-Forwarded-For-Y': ip}))

    def send_images_request(self, ip):
        return self.send_request(Fullreq("http://yandex.ru/images/search", headers={'X-Forwarded-For-Y': ip}))

    def send_web_request_with_spravka(self, ip, spravka):
        return self.send_request(Fullreq("http://yandex.ru/yandsearch?text=some", headers={'X-Forwarded-For-Y': ip, 'Cookie': 'spravka=%s' % spravka}))

    def make_search_requests(self, num_requests):
        ip = GenRandomIP()
        logs = []
        for i in range(num_requests):
            self.unified_agent.pop_daemon_logs()
            self.send_web_request(ip)
            line = self.unified_agent.wait_log_line_with_query(".*some.*")
            logs.append(line)
        return logs


class TestMatrixnetThresholds1(TestMatrixnetThresholdsBase):
    options = {
        "threshold": -100,
        "DisableBansByYql": 1,
        "MatrixnetFallbackProbability": 0.5,
    }

    def test_block_at_once(self):
        ip = GenRandomIP()
        self.unified_agent.pop_daemon_logs()
        self.send_web_request(ip)
        self.unified_agent.wait_log_line_with_query(".*some.*")  # wait until antirobot ban this address
        AssertCaptchaRedirect(self.send_web_request(ip))

    def test_usual_block(self):
        ip = GenRandomIP()
        AssertEventuallyTrue(lambda: self.send_images_request(ip).getcode() == 302)

    def test_robot_set_size_metric(self):
        ip = GenRandomIP()
        self.unified_agent.pop_daemon_logs()
        old_metric = self.antirobot.query_metric("robot_set_size_ahhh", id_type="other")
        self.send_web_request(ip)
        self.unified_agent.wait_log_line_with_query(".*some.*")  # wait until antirobot ban this address
        AssertCaptchaRedirect(self.send_web_request(ip))
        new_metric = self.antirobot.query_metric("robot_set_size_ahhh", id_type="other")
        assert new_metric - old_metric == 1


class TestMatrixnetThresholdsFallbackNoWizard(TestMatrixnetThresholdsBase):
    wizard_args = None
    options = {
        "DisableBansByFactors": 1,
    }

    def test_fallback_formula_no_wizard(self):
        logs = self.make_search_requests(20)
        assert [log for log in logs if not log["wizard_error"]] == []
        assert len([log for log in logs if log["matrixnet_fallback"]]) == 20
        assert [log for log in logs if not log["matrixnet_fallback"]] == []

    def test_fallback_formula_only_web(self):
        ip = GenRandomIP()
        self.unified_agent.pop_daemon_logs()
        self.send_request(Fullreq("http://yandex.ru/images/search?text=qweqwe", headers={'X-Forwarded-For-Y': ip}))
        log = self.unified_agent.wait_log_line_with_query(".*qweqwe.*")
        assert log["wizard_error"]
        assert not log["matrixnet_fallback"]


class TestMatrixnetThresholdsFallbackRandom(TestMatrixnetThresholdsBase):
    options = {
        "DisableBansByFactors": 1,
        "MatrixnetFallbackProbability": 0.5,
    }

    def test_fallback_formula_random(self):
        logs = self.make_search_requests(20)
        assert [log for log in logs if log["wizard_error"]] == []
        assert [log for log in logs if log["matrixnet_fallback"]] != []
        assert [log for log in logs if not log["matrixnet_fallback"]] != []


class TestMatrixnetThresholds2(TestMatrixnetThresholdsBase):
    options = {
        "threshold": 100,
        "DisableBansByYql": 1,
    }

    def test_never_block(self):
        ip = GenRandomIP()
        for _ in range(1000):
            AssertNotRobotResponse(self.send_request(Fullreq("http://market.yandex.ru/search", headers={'X-Forwarded-For-Y': ip})))


class TestMatrixnetThresholds3(TestMatrixnetThresholdsBase):
    options = {
        "SpravkaPenalty": 50,
        "processor_threshold_for_spravka_penalty@web": -100,
        "SpravkaIgnoreIfInRobotSet": 0,
    }

    captcha_args = ['--check', 'success']

    def test_block_at_Once(self):
        ip = '22.13.42.4'
        spravka = GetSpravkaForAddr(self, ip)
        self.unified_agent.pop_daemon_logs()
        self.send_web_request_with_spravka(ip, spravka)
        self.unified_agent.wait_log_line_with_query(".*some.*")  # wait until antirobot ban this address
        AssertCaptchaRedirect(self.send_web_request_with_spravka(ip, spravka))
