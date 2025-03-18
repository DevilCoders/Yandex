import pytest
import time

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util.asserts import AssertEventuallyTrue
from antirobot.daemon.arcadia_test.util import (
    VALID_L_COOKIE,
    VALID_I_COOKIE,
    VALID_FUID,
    GenRandomIP,
)


AMNESTY_INTERVAL_SHORT = 30  # seconds
AMNESTY_INTERVAL_MEDIUM = 10  # seconds
AMNESTY_INTERVAL_LONG = 3600  # seconds
REMOVE_EXPIRED_PERIOD = 1  # seconds
REBAN_ROBOTS_PERIOD = 3  # seconds
SYNC_LAG = 5  # seconds
OPTIONS = {
    "AuthorizeByFuid": 1,
    "AuthorizeByLCookie": 1,
    "AuthorizeByICookie": 1,
    "RemoveExpiredPeriod": REMOVE_EXPIRED_PERIOD,
    "DDosAmnestyPeriod": AMNESTY_INTERVAL_SHORT,
    "DDosFlag1BlockPeriod": AMNESTY_INTERVAL_SHORT,
    "DDosFlag2BlockPeriod": AMNESTY_INTERVAL_SHORT,
    # Чтобы не банить по формуле за частые однотипные запросы
    'DisableBansByFactors': 1,
    'ReBanRobotsPeriod': '0s',
}


class TestVerticalsConsistency1(AntirobotTestSuite):
    num_antirobots = 3
    options = dict(OPTIONS,
                   AmnestyIpInterval=AMNESTY_INTERVAL_LONG)

    def test_captcha_ban_eventually(self):
        ip = GenRandomIP()

        # Баним IP в одной вертикали
        self.antirobots[0].ban(ip)

        # Со временем этот IP должен оказаться забанен в остальных вертикалях
        for antirobot in self.antirobots:
            AssertEventuallyTrue(lambda: antirobot.is_banned(ip), secondsBetweenCalls=0.2)


class TestVerticalsConsistency2(AntirobotTestSuite):
    num_antirobots = 3
    options = OPTIONS

    def test_block_eventually(self):
        ip = GenRandomIP()
        block_duration = 3600  # seconds

        self.antirobots[0].block(ip, block_duration)

        # Небольшая деталь. Метод AntirobotInstance.block(Ip) блокирует адрес на конкретной машине.
        # Эта блокировка автоматически не пробрасывается на машину, на которой собирается информация
        # об Ip. Чтобы это произошло, на машину должен прийти запрос от Ip. Поэтому мы обязательно
        # должны вызвать self.antirobots[0].is_blocked() в цикле ниже.
        for antirobot in self.antirobots:
            AssertEventuallyTrue(lambda: antirobot.is_blocked(ip), secondsBetweenCalls=0.2)

    def test_unblock_eventually(self):
        ip = GenRandomIP()
        block_duration = 3  # seconds

        self.antirobots[0].block(ip, block_duration)
        # Чтобы пробросить блокировку. См. комментарий в BlockEventually
        self.antirobots[0].is_blocked(ip)

        time.sleep(SYNC_LAG + block_duration + REMOVE_EXPIRED_PERIOD)
        # По прошествии срока блокировки IP должен быть разблокирован во всех вертикалях
        for antirobot in self.antirobots:
            assert not antirobot.is_blocked(ip)


class TestVerticalsConsistencyWithLCookie(AntirobotTestSuite):
    num_antirobots = 3
    options = dict(OPTIONS,
                   SpravkaIgnoreIfInRobotSet=0,
                   AmnestyIpInterval="1d",
                   AmnestyFuidInterval=AMNESTY_INTERVAL_SHORT,
                   AmnestyICookieInterval=AMNESTY_INTERVAL_SHORT,
                   AmnestyLCookieInterval=AMNESTY_INTERVAL_SHORT,
                   SpravkaExpireInterval=AMNESTY_INTERVAL_SHORT)

    @pytest.mark.parametrize("ident_type, login_headers", [
        ("lcookie", {"Cookie": f"L={VALID_L_COOKIE}"}),
        ("icookie", {"X-Yandex-ICookie": VALID_I_COOKIE}),
        ("fuid",    {"Cookie": f"fuid01={VALID_FUID}"}),
        ("spravka", {}),
        ("ip",      {}),
    ])
    def test_captcha_unban(self, ident_type, login_headers):
        ip = GenRandomIP()
        if ident_type == "spravka":
            login_headers["Cookie"] = f"spravka={self.get_valid_spravka()}"

        # Баним в одной вертикали
        self.antirobots[0].ban(ip, headers=login_headers)
        ban_time = time.time()  # примерное время бана

        for antirobot in self.antirobots:
            # делаем по 1 запросу, чтобы быстрее забанить на всех вертикалях
            antirobot.is_banned(ip, headers=login_headers)

        for antirobot in self.antirobots:
            AssertEventuallyTrue(lambda: antirobot.is_banned(ip, headers=login_headers))
            if login_headers:
                assert not antirobot.is_banned(ip)

        ban_rest_time = time.time() - ban_time
        assert ban_rest_time < AMNESTY_INTERVAL_SHORT
        time.sleep(SYNC_LAG + (AMNESTY_INTERVAL_SHORT - ban_rest_time) + REMOVE_EXPIRED_PERIOD)

        # По прошествии срока должен быть разбанен во всех вертикалях, если пришел с кукой
        # если без куки, то должен остаться забанен
        for antirobot in self.antirobots:
            if login_headers:
                assert not antirobot.is_banned(ip, headers=login_headers)
                assert not antirobot.is_banned(ip)
            else:
                assert antirobot.is_banned(ip)


class TestVerticalsConsistencyRebanRobots(AntirobotTestSuite):
    num_antirobots = 3
    options = dict(OPTIONS,
                   AmnestyIpInterval=AMNESTY_INTERVAL_MEDIUM,
                   RemoveExpiredPeriod=REMOVE_EXPIRED_PERIOD,
                   ReBanRobotsPeriod=REBAN_ROBOTS_PERIOD)

    def test_reban_robots(self):
        ip = GenRandomIP()

        # Баним IP в одной вертикали
        self.antirobots[0].ban(ip)

        # Ждём пока наступит период перебана, но ещё не протухнит старый бан
        time.sleep(AMNESTY_INTERVAL_MEDIUM - REBAN_ROBOTS_PERIOD)

        # Перебаниваем в одной вертикали
        self.antirobots[0].ban(ip)

        # Ждём момента к которому гарантированно протух бы старый бан
        time.sleep(REBAN_ROBOTS_PERIOD + REMOVE_EXPIRED_PERIOD*2)

        # IP должен быть забанен везде
        for antirobot in self.antirobots:
            AssertEventuallyTrue(lambda: antirobot.is_banned(ip))
