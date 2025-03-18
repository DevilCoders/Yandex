from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import asserts
from antirobot.daemon.arcadia_test.util import GenRandomIP


AMNESTY_IP_INTERVAL = 60 * 60


class TestFaultTolerance(AntirobotTestSuite):
    def setup_method(self, method):
        super().setup_method(method)

        # Создаём три вертикали: в первых двух по одной машине, в третьей - две

        self.antirobots = self.enter(self.start_antirobots({
            "AmnestyIpInterval" : AMNESTY_IP_INTERVAL,
            "RemoveExpiredPeriod" : AMNESTY_IP_INTERVAL,
            "DDosAmnestyPeriod" : AMNESTY_IP_INTERVAL,
            "DDosFlag1BlockPeriod" : AMNESTY_IP_INTERVAL,
            "DDosFlag2BlockPeriod" : AMNESTY_IP_INTERVAL,
            "WizardHost": self.wizard.host,
        }, num_antirobots=4))

        self.verticals = [
            [self.antirobots[0]],
            [self.antirobots[1]],
            [self.antirobots[2], self.antirobots[3]],
        ]

    def teardown_method(self):
        for vertical in self.verticals:
            for daemon in vertical:
                daemon.terminate()

    def test_captcha_ban(self):
        ip = GenRandomIP()

        # Баним IP в одной вертикали
        self.verticals[0][0].ban(ip)

        # Дожидаемся, когда этот ip оказывается забанен в каждой вертикали
        for daemons in self.verticals:
            asserts.AssertEventuallyTrue(
                lambda: daemons[0].is_banned(ip),
                secondsBetweenCalls=0.2,
            )

        # Выключаем одну машину в третьей вертикали (ту, на которой IP уже забанен)
        self.verticals[2][0].terminate()

        # Со временем другая машина третьей вертикали должна забанить тестовый IP
        asserts.AssertEventuallyTrue(
            lambda: self.verticals[2][1].is_banned(ip),
            secondsBetweenCalls=0.2,
        )

    def test_block(self):
        ip = GenRandomIP()
        self.verticals[0][0].block(ip, AMNESTY_IP_INTERVAL)

        # Дожидаемся, когда этот ip оказывается заблокирован в каждой вертикали
        for daemons in self.verticals:
            asserts.AssertEventuallyTrue(
                lambda: daemons[0].is_blocked(ip),
                secondsBetweenCalls=0.2,
            )

        # Выключаем одну машину в третьей вертикали (ту, на которой IP уже заблокирован)
        self.verticals[2][0].terminate()

        # Со временем другая машина третьей вертикали должна заблокировать тестовый IP
        asserts.AssertEventuallyTrue(
            lambda: self.verticals[2][1].is_blocked(ip),
            secondsBetweenCalls=0.2,
        )
