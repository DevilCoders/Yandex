import pytest
import time
from pathlib import Path

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite

PROJECTID_MASK = "4fe2@2a02:6b8:c00::/40"


class TestProjectIdWhitelist(AntirobotTestSuite):
    DataDir = Path.cwd()
    YandexIps = DataDir / "yandex_ips"

    options = {
        "DisableBansByFactors": 1,
        "YandexIpsFile": YandexIps.name,
        "YandexIpsDir": DataDir,
    }

    @classmethod
    def setup_class(cls):
        with open(cls.YandexIps, "w") as f:
            print(PROJECTID_MASK, file=f)
        super().setup_class()

    @pytest.mark.parametrize('ip, banned', [
        ('22.13.42.4', True),
        ('2a02:6b8:c00:0:0:4fe2::', False),
        ('2a02:6b8:c00:0:0:5fe2::', True),
    ])
    def test_blocked_by_cbb_by_regexp(self, ip, banned):
        self.antirobot.ban(ip, check=False)
        time.sleep(5)
        assert self.antirobot.is_banned(ip) == banned
