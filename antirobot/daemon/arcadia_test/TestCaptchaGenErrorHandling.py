import pytest
import urllib
import json

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    captcha_page,
    Fullreq,
)
from antirobot.daemon.arcadia_test.util.asserts import (
    AssertNotRobotResponse,
    AssertCaptchaRedirect,
)


TRY_COUNT = 3


class TestCaptchaGenErrorHandling(AntirobotTestSuite):
    options = {
        "CaptchaCheckTimeout": "0.05s",
        "CaptchaGenTryCount": TRY_COUNT,
        "EventLogFrameSize": 0,  # все события сразу пишутся в файл eventlog'а
    }
    fury_args = ["--button-check", "fail"]

    def send_xml_partner_search(self):
        req = urllib.request.Request(r"http://xmlsearch.yandex.ru/xmlsearch?showmecaptcha=yes&text=%s" % captcha_page.FORCED_CAPTCHA_REQUEST)
        req.add_header("X-Real-IP", "1.2.3.4")
        return self.send_request(Fullreq(req))

    def send_ajax_search(self):
        return self.send_request(Fullreq(r"http://yandex.ru/yandsearch?ajax=1&text=%s" % captcha_page.FORCED_CAPTCHA_REQUEST))

    def get_captcha_key(self):
        self.restart_captcha(['--generate', 'correct'])
        return captcha_page.get_captcha_key(self)

    @pytest.mark.parametrize("captcha_gen_strategy", [
        "incorrect",
        "timeout",
    ])
    def test_xml_partner_search(self, captcha_gen_strategy):
        """
        Когда XML-партнёр посылает нам поисковый запрос, он ожидает либо SERP, либо XML с капчей.
        Поэтому если мы делаем редирект в ответ на /xmlsearch, и возникает ошибка генерации капчи,
        мы должны пропустить такой запрос на поиск.
        """
        self.restart_captcha(['--generate', captcha_gen_strategy])

        resp = self.send_xml_partner_search()
        AssertNotRobotResponse(resp)

    @pytest.mark.parametrize("captcha_gen_strategy", [
        "incorrect",
        "timeout",
    ])
    def test_ajax_search(self, captcha_gen_strategy):
        """
        При выполнении поискового запроса через AJAX клиент ждёт либо SERP, либо JSON с капчей.
        Поэтому если мы делаем редирект в ответ на AJAX-поиск, и возникает ошибка генерации капчи,
        мы должны пропустить такой запрос на поиск.
        """
        self.restart_captcha(['--generate', captcha_gen_strategy])

        resp = self.send_ajax_search()
        AssertNotRobotResponse(resp)

    @pytest.mark.parametrize("captcha_gen_strategy", [
        "incorrect",
        "timeout",
    ])
    def test_ordinary_show_captcha(self, captcha_gen_strategy):
        self.captcha.set_strategy("CheckStrategy", captcha_gen_strategy)

        page = captcha_page.HtmlCheckboxCaptchaPage(self, "yandex.ru")
        resp = self.send_request(Fullreq(page.get_check_captcha_url()))
        AssertCaptchaRedirect(resp)
        resp = self.send_request(Fullreq(resp.info()["Location"]))

        assert resp.getcode() == 302
        assert "/yandsearch" in resp.info()["Location"]
        assert resp.info()["Set-Cookie"].split("=")[0] == "spravka"

    @pytest.mark.parametrize("captcha_gen_strategy", [
        "incorrect",
        "timeout",
    ])
    def test_xml_partner_check_captcha(self, captcha_gen_strategy):
        """
        В ответ на запрос /xcheckcaptcha XML-партнёр ожидает от нас либо новую капчу, либо сообщение
        об успешном вводе. Поэтому если мы делаем редирект в ответ на неправильно введённую капчу,
        и возникает ошибка генерации капчи, мы должны ответить сообщением об успешном вводе капчи.
        """

        captcha_key = self.get_captcha_key()

        self.restart_captcha(['--generate', captcha_gen_strategy, '--check', 'fail'])

        req = urllib.request.Request(r"http://yandex.ru/xcheckcaptcha?key=%s&rep=123" % captcha_key)
        req.add_header("X-Real-IP", "1.2.3.4")
        resp = self.send_request(Fullreq(req))
        assert resp.getcode() == 200
        assert "Set-Cookie" in resp.info()
        assert resp.info()["Set-Cookie"].split("=")[0] == "spravka"

    @pytest.mark.parametrize("captcha_gen_strategy", [
        "incorrect",
        "timeout",
    ])
    def test_ajax_check_captcha(self, captcha_gen_strategy):
        """
        В ответ на запрос /checkcaptchajson клиент ожидает от нас либо новую капчу, либо сообщение
        об успешном вводе. Поэтому если мы делаем редирект в ответ на неправильно введённую капчу,
        и возникает ошибка генерации капчи, мы должны ответить сообщением об успешном вводе капчи.
        """

        captcha_key = self.get_captcha_key()

        self.restart_captcha(['--generate', captcha_gen_strategy, '--check', 'fail'])

        resp = self.send_request(Fullreq(r"http://yandex.ru/checkcaptchajson?key=%s&rep=123" % captcha_key))
        assert resp.getcode() == 200
        vals = json.loads(resp.read())
        assert "spravka" in vals

    def test_logging_for_gen_error(self):
        self.restart_captcha(['--generate', 'timeout'])
        self.unified_agent.pop_event_logs()

        resp = self.send_xml_partner_search()
        AssertNotRobotResponse(resp)

        events = self.unified_agent.get_event_logs(["TGeneralMessage", "TRequestGeneralMessage"])

        access_failure_count = len([event for event in events if "Captcha access error" in event.Event.Message])
        captcha_gen_error_count = len([event for event in events if "Captcha generate error" in event.Event.Message])

        assert access_failure_count == TRY_COUNT
        assert captcha_gen_error_count == 1

    def test_logging_for_check_error(self):
        captcha_key = self.get_captcha_key()
        self.unified_agent.pop_event_logs()

        self.restart_captcha(['--generate', 'correct', '--check', 'timeout'])

        resp = self.send_request(Fullreq(r"http://yandex.ru/checkcaptchajson?key=%s&rep=123" % captcha_key))
        assert resp.getcode() == 200

        events = self.unified_agent.get_event_logs(["TGeneralMessage"])
        access_failure_count = len([event for event in events if "Captcha check error" in event.Event.Message])

        captcha_checks = self.unified_agent.get_event_logs(["TCaptchaCheck"])
        captcha_checks_count = len(captcha_checks)

        assert access_failure_count == 1
        assert captcha_checks_count == 1

        captcha_check_event = captcha_checks[0]
        was_captcha_connect_error = captcha_check_event.Event.CaptchaConnectError
        assert bool(was_captcha_connect_error)

    def test_captcha_success_if_check_fails(self):
        captcha_key = self.get_captcha_key()

        self.restart_captcha(['--generate', 'correct', '--check', 'timeout'])

        resp = self.send_request(Fullreq(r"http://yandex.ru/checkcaptchajson?key=%s&rep=123" % captcha_key))
        assert resp.getcode() == 200

        vals = json.loads(resp.read())
        assert "captcha" in vals
        vals = vals["captcha"]
        assert "status" in vals
        assert vals["status"] == "success"
