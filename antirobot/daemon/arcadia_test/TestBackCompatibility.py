import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util.asserts import AssertEventuallyTrue
from antirobot.daemon.arcadia_test.util import GenRandomIP


OPTIONS = {
    "RemoveExpiredPeriod": 3600,
    "DDosAmnestyPeriod": 3600,
    "DDosFlag1BlockPeriod": 3600,
    "DDosFlag2BlockPeriod": 3600,
    "ReBanRobotsPeriod": "0s",
    "DbSyncInterval": "5s",
    "DisableBansByFactors": 1,
}
NUM_IPS = 150
NUM_ANTIROBOTS = 2
NUM_OLD_ANTIROBOTS = 2


class TestBackCompatibility(AntirobotTestSuite):
    num_antirobots = NUM_ANTIROBOTS
    num_old_antirobots = NUM_OLD_ANTIROBOTS
    options = dict(OPTIONS, AmnestyIpInterval=3600)

    @pytest.mark.parametrize("antirobot_idx", list(range(NUM_ANTIROBOTS + NUM_OLD_ANTIROBOTS)))
    def test_back_compat_base(self, antirobot_idx):
        ips = [GenRandomIP(v=4) for i in range(NUM_IPS // 2)] + [GenRandomIP(v=6) for i in range((NUM_IPS + 1) // 2)]
        for ip in ips:
            self.antirobots[antirobot_idx].ban(ip, check=False)

        for antirobot in self.antirobots:
            for ip in ips:
                # делаем по 1 запросу, чтобы все баны начали доезжать
                # ждать по одному в AssertEventuallyTrue долго
                antirobot.is_banned(ip)

        # Со временем этот IP должен оказаться забанен во всех
        for i, antirobot in enumerate(self.antirobots):
            for ip in ips:
                AssertEventuallyTrue(lambda: antirobot.is_banned(ip), secondsBetweenCalls=0.2)
            assert antirobot.is_alive()
