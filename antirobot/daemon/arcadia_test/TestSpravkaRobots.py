from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    GenRandomIP,
    Fullreq,
    IsCaptchaRedirect,
)


SEARCH_TEXT = "helloworld"
MIN_REQUESTS_WITH_SPRAVKA = 6
SPRAVKA_REQUESTS_LIMIT = 15
OPTIONS = {
    "MinRequestsWithSpravka": MIN_REQUESTS_WITH_SPRAVKA,
    "SpravkaPenalty": 0,
}


class TestSpravkaRobotsBase(AntirobotTestSuite):
    def send_web_request(self, ip, spravka=""):
        return self.send_request(Fullreq(
            f"http://yandex.ru/yandsearch?text={SEARCH_TEXT}",
            headers={"X-Forwarded-For-Y": ip, "Cookie": f"spravka={spravka}"}
        ))

    def send_request_and_wait_log(self, ip, spravka):
        self.unified_agent.pop_daemon_logs()
        r = self.send_web_request(ip, spravka)
        log_line = self.unified_agent.wait_log_line_with_query(f".*{SEARCH_TEXT}.*")
        return r, log_line


class TestSpravkaRobotsThreshold0(TestSpravkaRobotsBase):
    options = dict(OPTIONS,
                   threshold=0,
                   SpravkaRequestsLimit=SPRAVKA_REQUESTS_LIMIT)

    def test_spravka_ignore(self):
        ip = GenRandomIP()
        spravka = self.get_valid_spravka()

        self.antirobot.ban(ip)

        spravka_req_count = 0
        num_requests = MIN_REQUESTS_WITH_SPRAVKA + 10
        for i in range(num_requests):
            r, log = self.send_request_and_wait_log(ip, spravka)
            is_spravka = log["ident_type"].startswith("2-")
            spravka_req_count += int(is_spravka)
            assert is_spravka == (not log["spravka_ignored"])
            assert IsCaptchaRedirect(r) == (not is_spravka), f"at {i}-th iteration"

        # Проверяем, что сделали MIN_REQUESTS_WITH_SPRAVKA + 1 запросов со справкой
        # все остальные должны быть с ident_type=ip и вести на капчу
        assert spravka_req_count == MIN_REQUESTS_WITH_SPRAVKA + 1


class TestSpravkaRobotsNoThreshold(TestSpravkaRobotsBase):
    options = dict(OPTIONS,
                   SpravkaRequestsLimit=SPRAVKA_REQUESTS_LIMIT)

    def test_spravka_ignore_and_unban(self):
        ip = GenRandomIP()
        spravka = self.get_valid_spravka()

        spravka_req_count = 0
        num_requests = MIN_REQUESTS_WITH_SPRAVKA + 2
        for i in range(num_requests):
            r, log = self.send_request_and_wait_log(ip, spravka)
            is_spravka = log["ident_type"].startswith("2-")
            spravka_req_count += int(is_spravka)
            assert is_spravka == (not log["spravka_ignored"])
            assert not IsCaptchaRedirect(r), f"at {i}-th iteration"

        # Здесь проверяем, что все запросы не ведут на капчу
        # сначала работает справка, потом переход на id ident_type
        # threshold обычный, на 1 запрос точно хватит
        assert spravka_req_count == MIN_REQUESTS_WITH_SPRAVKA + 1


class TestSpravkaRobotsNoFactorsBan(TestSpravkaRobotsBase):
    options = dict(OPTIONS,
                   DisableBansByFactors=1,
                   SpravkaRequestsLimit=SPRAVKA_REQUESTS_LIMIT)

    def test_spravka_requests_limit(self):
        assert SPRAVKA_REQUESTS_LIMIT > MIN_REQUESTS_WITH_SPRAVKA

        ip = GenRandomIP()
        spravka = self.get_valid_spravka()

        self.antirobot.ban(ip)

        spravka_req_count = 0
        num_requests = SPRAVKA_REQUESTS_LIMIT + 3
        for i in range(num_requests):
            r, log = self.send_request_and_wait_log(ip, spravka)
            is_spravka = log["ident_type"].startswith("2-")
            spravka_req_count += int(is_spravka)
            assert is_spravka == (not log["spravka_ignored"])
            assert IsCaptchaRedirect(r) == (not is_spravka), f"at {i}-th iteration"
            assert bool(log["force_ban"]) == (i == SPRAVKA_REQUESTS_LIMIT)

        # первые SPRAVKA_REQUESTS_LIMIT + 1 проходят по справке
        # после чего справка банится и юзер переходит на ident_type=ip
        assert spravka_req_count == SPRAVKA_REQUESTS_LIMIT + 1
