import pytest
import time
from pathlib import Path

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    GenRandomIP,
    captcha_page,
    spravka,
)
from antirobot.daemon.arcadia_test.util.asserts import (
    AssertCaptchaRedirect,
    AssertRedirectNotToCaptcha
)


HANDLE_WATCHER_POLL_INTERVAL = 0.1


class TestCaptchaFury(AntirobotTestSuite):
    captcha_args = ["--generate", "correct"]
    Controls = Path.cwd() / "controls"
    options = {
        "CaptchaCheckTimeout": "0.05s",
        "CaptchaGenTryCount": 3,
        "EventLogFrameSize": 0,  # все события сразу пишутся в файл eventlog'а
        "FuryBaseTimeout": "0.1s",
        "HandleStopFuryForAllFilePath": Controls / "stop_fury_for_all",
        "HandleStopFuryPreprodForAllFilePath": Controls / "stop_fury_preprod_for_all",
        "HandleWatcherPollInterval": "%.1fs" % HANDLE_WATCHER_POLL_INTERVAL,
    }

    @classmethod
    def setup_class(cls):
        cls.Controls.mkdir(exist_ok=True)
        super().setup_class()

    def do_check(self, expected_success, captcha_answer, content=None):
        captcha = captcha_page.HtmlCaptchaPage(self, "yandex.ru")
        ip = GenRandomIP()
        daemon = self.antirobots[0]
        captcha_redirect = spravka.DoCheckCaptcha(daemon, ip, captcha, rep=captcha_answer, content=content)

        if expected_success:
            AssertRedirectNotToCaptcha(captcha_redirect)
        else:
            AssertCaptchaRedirect(captcha_redirect)

    def is_success(self, captcha_answer, captcha_strategy, fury_strategy):
        if not captcha_answer:
            return False

        if captcha_strategy == "as_rep":
            captcha_strategy = "success" if captcha_answer == "ok" else "fail"

        if captcha_strategy == "error":
            return False
        if captcha_strategy in ("incorrect", "timeout"):
            return True
        if captcha_strategy == "fail":
            return fury_strategy == "success"
        if captcha_strategy == "success":
            return fury_strategy != "fail"
        assert False, f"Unknown captcha_strategy='{captcha_strategy}'"

    @pytest.mark.parametrize("expected_success, captcha_check_strategy, fury_check_strategy, captcha_answer", [
        (False, "fail",      "fail",      "123"),
        (True,  "fail",      "success",   "123"),
        (False, "success",   "fail",      "123"),
        (True,  "success",   "success",   "123"),
        (False, "fail",      "as_image",  "123"),
        (True,  "success",   "as_image",  "123"),

        (True,  "timeout",   "as_image",  "123"),
        (False, "timeout",   "as_image",  ""),  # проверим, что запроса не было в капчу и он посчитался за incorrect

        (True,  "incorrect", "as_image",  "123"),
        (False, "fail",      "timeout",   "123"),
        (True,  "success",   "timeout",   "123"),
        (False, "fail",      "incorrect", "123"),
        (True,  "success",   "incorrect", "123"),

        (False, "error",     "as_image",  "123"),
    ])
    def test_pass_captcha(self, expected_success, captcha_check_strategy, fury_check_strategy, captcha_answer):
        captcha_ok = captcha_answer != "" and captcha_check_strategy in ("success", "timeout", "incorrect")

        self.captcha.set_strategy("CheckStrategy", captcha_check_strategy)
        self.fury.set_strategy("Strategy", fury_check_strategy)
        assert expected_success == self.is_success(captcha_answer, captcha_check_strategy, fury_check_strategy)

        for fury_preprod_check_strategy in ("fail", "success", "incorrect", "timeout"):
            self.fury_preprod.set_strategy("Strategy", fury_preprod_check_strategy)

            stats_before = self.antirobot.get_stats()
            self.do_check(expected_success, captcha_answer)
            time.sleep(0.5)
            stats_after = self.antirobot.get_stats()

            is_failed_request = lambda s: s in ("incorrect", "timeout") and captcha_check_strategy not in ("incorrect", "timeout", "error")
            expected_preprod_success = self.is_success(captcha_answer, captcha_check_strategy, fury_preprod_check_strategy)

            assert_changed_if = lambda cond, metric: self._assert_changed_if(cond, metric, stats_before, stats_after)

            assert_changed_if(is_failed_request(fury_check_strategy), "fury_check_errors_deee")
            assert_changed_if(is_failed_request(fury_preprod_check_strategy), "fury_preprod_check_errors_deee")
            assert_changed_if(expected_success, "captcha_correct_inputs_deee")
            assert_changed_if(not expected_success, "captcha_incorrect_inputs_deee")
            assert_changed_if(expected_preprod_success, "captcha_preprod_correct_inputs_deee")
            assert_changed_if(not expected_preprod_success, "captcha_preprod_incorrect_inputs_deee")

            assert_changed_if(captcha_ok and not expected_success, "captcha_fury_ok_to_fail_changes_deee")
            assert_changed_if(not captcha_ok and expected_success, "captcha_fury_fail_to_ok_changes_deee")
            assert_changed_if(captcha_ok and not expected_preprod_success, "captcha_fury_preprod_ok_to_fail_changes_deee")
            assert_changed_if(not captcha_ok and expected_preprod_success, "captcha_fury_preprod_fail_to_ok_changes_deee")

            assert_changed_if(captcha_check_strategy == "error" or captcha_answer == "", "captcha_check_bad_requests_deee")

    def _assert_changed_if(self, cond, metric, stats_before, stats_after):
        before = self.antirobot.query_metric(metric, stats=stats_before)
        after = self.antirobot.query_metric(metric, stats=stats_after)
        assert before + int(cond) == after, f"For metric {metric}"

    def write_its_and_wait(self, prod_value, preprod_value):
        with open(self.options["HandleStopFuryForAllFilePath"], "w") as file:
            file.write(prod_value)
        with open(self.options["HandleStopFuryPreprodForAllFilePath"], "w") as file:
            file.write(preprod_value)
        time.sleep(HANDLE_WATCHER_POLL_INTERVAL + 0.1)

    def test_pass_captcha_no_fury_its(self):
        self.captcha.set_strategy("CheckStrategy", "success")
        self.fury.set_strategy("Strategy", "fail")

        def get_metric(s):
            time.sleep(0.5)
            return self.antirobot.get_metric(f"stopped_{s}_requests_deee")

        stopped_cnt = 0
        stopped_preprod_cnt = 0

        stopped_prod_old = get_metric("fury")
        stopped_preprod_old = get_metric("fury_preprod")
        self.do_check(expected_success=False, captcha_answer="zxc")
        assert stopped_prod_old + stopped_cnt == get_metric("fury")
        assert stopped_preprod_old + stopped_preprod_cnt == get_metric("fury_preprod")

        self.write_its_and_wait("enable", "")
        stopped_cnt += 2
        self.do_check(expected_success=True, captcha_answer="zxc")
        assert stopped_prod_old + stopped_cnt == get_metric("fury")
        assert stopped_preprod_old + stopped_preprod_cnt == get_metric("fury_preprod")

        self.write_its_and_wait("", "")
        self.do_check(expected_success=False, captcha_answer="zxc")
        assert stopped_prod_old + stopped_cnt == get_metric("fury")
        assert stopped_preprod_old + stopped_preprod_cnt == get_metric("fury_preprod")

        self.write_its_and_wait("", "enable")
        stopped_preprod_cnt += 2
        self.do_check(expected_success=False, captcha_answer="zxc")
        assert stopped_prod_old + stopped_cnt == get_metric("fury")
        assert stopped_preprod_old + stopped_preprod_cnt == get_metric("fury_preprod")

        self.write_its_and_wait("enable", "enable")
        stopped_cnt += 2
        stopped_preprod_cnt += 2
        self.do_check(expected_success=True, captcha_answer="zxc")
        assert stopped_prod_old + stopped_cnt == get_metric("fury")
        assert stopped_preprod_old + stopped_preprod_cnt == get_metric("fury_preprod")

        self.write_its_and_wait("", "")
        self.do_check(expected_success=False, captcha_answer="zxc")
        assert stopped_prod_old + stopped_cnt == get_metric("fury")
        assert stopped_preprod_old + stopped_preprod_cnt == get_metric("fury_preprod")

    def test_log_captcha_warnings(self):
        self.captcha.set_strategy("CheckStrategy", "error")
        self.fury.set_strategy("Strategy", "as_image")

        self.unified_agent.pop_event_logs()
        self.do_check(False, "123")

        captcha_checks = self.unified_agent.get_event_logs(["TCaptchaCheck"])

        assert len(captcha_checks) == 2
        assert "test error; test error description" in captcha_checks[1].Event.WarningMessages

    @pytest.mark.parametrize("expected_success, content", [
        (True, ""),
        (False, "test=true"),
    ])
    def test_checkbox_test_flag(self, expected_success, content):
        self.fury.set_strategy("ButtonStrategy", "success")

        captcha = captcha_page.HtmlCheckboxCaptchaPage(self, "yandex.ru")
        ip = GenRandomIP()
        daemon = self.antirobots[0]
        captcha_redirect = spravka.DoCheckCaptcha(daemon, ip, captcha, rep="", content=content)

        if expected_success:
            AssertRedirectNotToCaptcha(captcha_redirect)
        else:
            AssertCaptchaRedirect(captcha_redirect)
