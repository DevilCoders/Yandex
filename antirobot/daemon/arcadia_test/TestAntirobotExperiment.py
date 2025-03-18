import time
import urllib.request
import urllib.parse
from pathlib import Path

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    GenRandomIP
)

WATCHER_PERIOD = 0.1

VALID_I_COOKIES = [9629147541626103949, 7171658571626103949, 3729438441626103889, 2260836281626103885,
                   9883499601626103949, 1966573081626103949, 4446214731626103949, 7502263761626103949,
                   2812990071626103881, 7051310451626103949, 5972913941626103949, 1527744531626103950,
                   2663737591626103893, 1405659111626103950, 7812418331626103950, 1725228951626103933,
                   8042288601626103950, 7987700561626103950, 4477222321626103950, 9918490981626103950,
                   4278796231626103950, 1747719231626103949, 5761945891626103924, 3205733231626103949,
                   1050156501626103949, 2215288521626103949, 3199852211626103949, 6419000951626103949,
                   5312612691626103950, 1850825661626103950, 7400978941626103950, 4526054561626103950,
                   5586431561608982609, 2636173221626103950, 2512621791626103950, 9501617591626103949,
                   7421112651626025858, 2900319021626103950, 9909179631626103950, 1371964281626103914,
                   2260798601626103946, 6214817721626095383, 6920534081626103947, 4806957391626103947,
                   7754001241626103947, 7189497521625983054, 6395507671625793543, 8661981611626103935]

SKIP_SMART_CAPTCHA_TEST_ID1 = 387278
SKIP_SMART_CAPTCHA_TEST_ID2 = 387277

CATBOOST_WHITELIST_TEST_ID1 = 404627
CATBOOST_WHITELIST_TEST_ID2 = 404629

BAN_TEXT = urllib.request.quote("Капчу!")
BAN_REQ = "http://yandex.ru/search?text=%s&param1=buble1-goom" % BAN_TEXT
NOT_BAN_REQ = "http://yandex.ru/search?text=cat&param1=buble1-goom"

REQUEST_FOR_BAN = Fullreq(
    BAN_REQ,
    headers={
        "X-Forwarded-For-Y": "1.2.3.1",
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
        "Host": "yandex.ru",
        "Referer": "http://www.yandex.ru/referer",
        "X-Yandex-ICookie": 9629147541626103949,
    }
)

REQUEST_NOT_FOR_BAN = Fullreq(
    NOT_BAN_REQ,
    headers={
        "X-Forwarded-For-Y": "1.2.3.1",
        "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
        "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
        "Host": "yandex.ru",
        "Referer": "http://www.yandex.ru/referer",
        "X-Yandex-ICookie": 9629147541626103949,
    }
)


class TestAntirobotSkipSmartCaptchaExperiment(AntirobotTestSuite):
    Controls = Path.cwd() / "controls"

    options = {
        "HandleAntirobotDisableExperimentsPath": Controls / "disable_experiments",
        "DisableBansByFactors": 1,
        "HandleWatcherPollInterval": WATCHER_PERIOD,
    }

    @classmethod
    def setup_class(cls):
        cls.Controls.mkdir(exist_ok=True)
        super().setup_class()

    def setup_method(self, method):
        super().setup_method(method)

        for opt_name in ["HandleAntirobotDisableExperimentsPath"]:
            with open(self.options[opt_name], "w") as f:
                f.write("")

        time.sleep(WATCHER_PERIOD + 0.1)

    def write_its(self, opt_name, value):
        with open(self.options[opt_name], "w") as f:
            f.write(value)

        time.sleep(WATCHER_PERIOD + 0.1)

    def check_correct(self, resp):
        header = self.unified_agent.wait_log_line_with_query(f'.*{BAN_TEXT}.*')["experiments_test_id"]
        token_pos = resp.info()["Location"].find("t=")

        if SKIP_SMART_CAPTCHA_TEST_ID1 in header:
            assert resp.info()["Location"][token_pos + 2] == "3"
        elif SKIP_SMART_CAPTCHA_TEST_ID2 in header:
            assert resp.info()["Location"][token_pos + 2] == "2"
        else:
            raise Exception("Inavlid params")

    def test_start_and_stop_antirobot_experiment(self):
        # По умолчанию эксперимент запущен
        for ICOOKIE in VALID_I_COOKIES:
            headers = {
                "X-Forwarded-For-Y": GenRandomIP(),
                "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
                "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
                "Host": "yandex.ru",
                "Referer": "http://www.yandex.ru/referer",
                "X-Yandex-ICookie": f"{ICOOKIE}"
            }
            resp = self.antirobot.send_request(Fullreq(BAN_REQ, headers))
            self.check_correct(resp)

        # Любой запрос с веба имеет хедер
        resp = self.antirobot.send_request(REQUEST_NOT_FOR_BAN)
        daemon_log = self.unified_agent.wait_log_line_with_query('.*cat.*')
        assert "experiments_test_id" in daemon_log

        # Останавливаем эксперимент
        self.write_its("HandleAntirobotDisableExperimentsPath", "enable")

        resp = self.antirobot.send_request(REQUEST_FOR_BAN)
        daemon_log = self.unified_agent.wait_log_line_with_query(f'.*{BAN_TEXT}.*')

        assert "experiments_test_id" not in daemon_log

        token_pos = resp.info()["Location"].find("t=")
        assert resp.info()["Location"][token_pos + 2] == "2"

    def test_requests_without_cookie(self):
        for i in range(50):
            headers = {
                "X-Forwarded-For-Y": GenRandomIP(),
                "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
                "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
                "Host": "yandex.ru",
                "Referer": "http://www.yandex.ru/referer",
            }
            resp = self.antirobot.send_request(Fullreq(BAN_REQ, headers))
            self.check_correct(resp)

    def test_getting_into_the_experiment(self):
        # Проверка что какие-то запросы правда попадают под эксперимент
        # Это две куки, одна из которых попадает в выборку при эксперименте с солью "b98f9488" и 5% выборки и одна которая не попадает
        IN_EXP_ICOOKIE = 9096433691626594722
        NOT_IN_EXP_ICOOKIE = 2998812261626594722

        in_exp_headers = {
            "X-Forwarded-For-Y": GenRandomIP(),
            "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
            "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
            "Host": "yandex.ru",
            "Referer": "http://www.yandex.ru/referer",
            "X-Yandex-ICookie": f"{IN_EXP_ICOOKIE}"
        }

        not_in_exp_headers = {
            "X-Forwarded-For-Y": GenRandomIP(),
            "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
            "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
            "Host": "yandex.ru",
            "Referer": "http://www.yandex.ru/referer",
            "X-Yandex-ICookie": f"{NOT_IN_EXP_ICOOKIE}"
        }

        # проверка что он правда попал под эксперимент
        in_exp_resp = self.antirobot.send_request(Fullreq(BAN_REQ, in_exp_headers))
        in_exp_header_test_id = self.unified_agent.wait_log_line_with_query(f'.*{BAN_TEXT}.*')["experiments_test_id"]
        in_exp_token_pos = in_exp_resp.info()["Location"].find("t=")
        assert SKIP_SMART_CAPTCHA_TEST_ID1 in in_exp_header_test_id
        assert in_exp_resp.info()["Location"][in_exp_token_pos + 2] == "3"

        # не попал под эксперимент
        not_in_exp_resp = self.antirobot.send_request(Fullreq(BAN_REQ, not_in_exp_headers))
        not_in_exp_header_test_id = self.unified_agent.wait_log_line_with_query(f'.*{BAN_TEXT}.*')["experiments_test_id"]
        not_in_exp_token_pos = not_in_exp_resp.info()["Location"].find("t=")
        assert SKIP_SMART_CAPTCHA_TEST_ID2 in not_in_exp_header_test_id
        assert not_in_exp_resp.info()["Location"][not_in_exp_token_pos + 2] == "2"


class TestAntirobotCatboostWhitelistExperiment(AntirobotTestSuite):
    Controls = Path.cwd() / "controls"

    options = {
        "HandleAntirobotDisableExperimentsPath": Controls / "disable_experiments",
        "HandleWatcherPollInterval": WATCHER_PERIOD,
        "cacher_threshold@web": -100000
    }

    @classmethod
    def setup_class(cls):
        cls.Controls.mkdir(exist_ok=True)
        super().setup_class()

    def setup_method(self, method):
        super().setup_method(method)

        for opt_name in ["HandleAntirobotDisableExperimentsPath"]:
            with open(self.options[opt_name], "w") as f:
                f.write("")

        time.sleep(WATCHER_PERIOD + 0.1)

    def write_its(self, opt_name, value):
        with open(self.options[opt_name], "w") as f:
            f.write(value)

        time.sleep(WATCHER_PERIOD + 0.1)

    def test_getting_into_the_experiment(self):
        # Проверка что какие-то запросы правда попадают под эксперимент
        # Это два ip, один из которых попадает в выборку при эксперименте с солью "57d704a3" и 10% выборки и один который не попадает
        # Кешерная формула всегда возвращает True из-за низкого порога в конфиге
        IN_EXP_IP = "222.180.130.44"
        NOT_IN_EXP_IP = "84.242.9.252"

        self.antirobot.ban(IN_EXP_IP, "yandsearch", False)
        self.antirobot.ban(NOT_IN_EXP_IP, "yandsearch", False)
        time.sleep(5)

        in_exp_headers = {
            "X-Forwarded-For-Y": f"{IN_EXP_IP}",
            "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
            "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
            "Host": "yandex.ru",
            "Referer": "http://www.yandex.ru/referer"
        }

        not_in_exp_headers = {
            "X-Forwarded-For-Y": f"{NOT_IN_EXP_IP}",
            "X-Req-Id": "1346776354215268-879007856167401084457064-6-025",
            "User-Agent": "Opera/9.80 (Windows NT 6.1; U; MRA 5.10 (build 5244); ru) Testo/2.10.289 Version/12.02",
            "Host": "yandex.ru",
            "Referer": "http://www.yandex.ru/referer"
        }

        # проверка что он правда попал под эксперимент
        self.antirobot.send_request(Fullreq(NOT_BAN_REQ, in_exp_headers))
        in_exp_daemon_log = self.unified_agent.wait_log_line_with_query('.*cat.*')
        assert in_exp_daemon_log["catboost_whitelist"] is False
        assert CATBOOST_WHITELIST_TEST_ID1 in in_exp_daemon_log["experiments_test_id"]

        # не попал под эксперимент
        self.antirobot.send_request(Fullreq(NOT_BAN_REQ, not_in_exp_headers))
        not_in_exp_daemon_log = self.unified_agent.wait_log_line_with_query('.*cat.*')
        assert not_in_exp_daemon_log["catboost_whitelist"] is True
        assert CATBOOST_WHITELIST_TEST_ID2 in not_in_exp_daemon_log["experiments_test_id"]
