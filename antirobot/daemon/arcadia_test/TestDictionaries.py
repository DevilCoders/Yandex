import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import GenRandomIP


class TestDictionaries(AntirobotTestSuite):
    options = {
        "DisableBansByFactors": 1,
        "CacherRandomFactorsProbability": 1.0
    }

    def test_market_stats_ja3(self):
        ip = GenRandomIP()
        self.send_fullreq("http://yandex.ru/search?text=111",
                          headers={
                              "X-Forwarded-For-Y": ip,
                              "X-Antirobot-Service-Y": "market",
                              "X-Antirobot-MayBanFor-Y": 1,
                              "X-Yandex-Ja3": "769,4-5-47-51-50-10-22-19-9-21-18-3-8-20-17-255,,,"
                          })
        factors = self.unified_agent.wait_event_logs(["TCacherFactors"])[0].Event
        assert factors.MarketJwsStateIsDefaultExpiredRatio == 10
        assert factors.MarketJwsStateIsDefaultRatio == 20
        assert factors.MarketJwsStateIsInvalidRatio == 30
        assert factors.MarketJwsStateIsSuspExpiredRatio == 40
        assert factors.MarketJwsStateIsSuspRatio == 50
        assert factors.MarketJwsStateIsValidExpiredRatio == 60
        assert factors.MarketJwsStateIsValidRatio == 70
        assert factors.MarketJa3BlockedCntRatio == 2
        assert factors.MarketJa3CatalogReqsCntRatio == 3
        assert factors.MarketJa3EnemyCntRatio == 4
        assert factors.MarketJa3EnemyRedirectsCntRatio == 5
        assert factors.MarketJa3FuidCntRatio == 6
        assert factors.MarketJa3HostingCntRatio == 7
        assert factors.MarketJa3IcookieCntRatio == 8
        assert factors.MarketJa3Ipv4CntRatio == 9
        assert factors.MarketJa3Ipv6CntRatio == 10
        assert factors.MarketJa3LoginCntRatio == 11
        assert factors.MarketJa3MobileCntRatio == 12
        assert factors.MarketJa3OtherHandlesReqsCntRatio == 13
        assert factors.MarketJa3ProductReqsCntRatio == 14
        assert factors.MarketJa3ProxyCntRatio == 15
        assert factors.MarketJa3RefererIsEmptyCntRatio == 16
        assert factors.MarketJa3RefererIsNotYandexCntRatio == 17
        assert factors.MarketJa3RefererIsYandexCntRatio == 18
        assert factors.MarketJa3RobotsCntRatio == 19
        assert factors.MarketJa3SearchReqsCntRatio == 20
        assert factors.MarketJa3SpravkaCntRatio == 21
        assert factors.MarketJa3TorCntRatio == 22
        assert factors.MarketJa3VpnCntRatio == 23
        assert factors.MarketJa3YndxIpCntRatio == 24

    def test_market_stats_user_agent(self):
        ip = GenRandomIP()
        self.send_fullreq("http://yandex.ru/search?text=111",
                          headers={
                              "X-Forwarded-For-Y": ip,
                              "X-Antirobot-Service-Y": "market",
                              "X-Antirobot-MayBanFor-Y": 1,
                              "User-Agent": "Mozilla/5.0 (Linux; Android 10; EML-L29) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/95.0.4614.0 Mobile Safari/537.36"
                          })
        factors = self.unified_agent.wait_event_logs(["TCacherFactors"])[0].Event
        assert factors.MarketUABlockedCntRatio == 2
        assert factors.MarketUACatalogReqsCntRatio == 3
        assert factors.MarketUAEnemyCntRatio == 4
        assert factors.MarketUAEnemyRedirectsCntRatio == 5
        assert factors.MarketUAFuidCntRatio == 6
        assert factors.MarketUAHostingCntRatio == 7
        assert factors.MarketUAIcookieCntRatio == 8
        assert factors.MarketUAIpv4CntRatio == 9
        assert factors.MarketUAIpv6CntRatio == 10
        assert factors.MarketUALoginCntRatio == 11
        assert factors.MarketUAMobileCntRatio == 12
        assert factors.MarketUAOtherHandlesReqsCntRatio == 13
        assert factors.MarketUAProductReqsCntRatio == 14
        assert factors.MarketUAProxyCntRatio == 15
        assert factors.MarketUARefererIsEmptyCntRatio == 16
        assert factors.MarketUARefererIsNotYandexCntRatio == 17
        assert factors.MarketUARefererIsYandexCntRatio == 18
        assert factors.MarketUARobotsCntRatio == 19
        assert factors.MarketUASearchReqsCntRatio == 20
        assert factors.MarketUASpravkaCntRatio == 21
        assert factors.MarketUATorCntRatio == 22
        assert factors.MarketUAVpnCntRatio == 23
        assert factors.MarketUAYndxIpCntRatio == 24

    @pytest.mark.parametrize("ip", (
        "1.0.148.18",
        "2a00:1fa0:440d:6a2c::1da0",
    ))
    def test_market_stats_subnet(self, ip):
        self.send_fullreq("http://yandex.ru/search?text=111",
                          headers={
                              "X-Forwarded-For-Y": ip,
                              "X-Antirobot-Service-Y": "market",
                              "X-Antirobot-MayBanFor-Y": 1,
                          })
        factors = self.unified_agent.wait_event_logs(["TCacherFactors"])[0].Event
        assert factors.MarketSubnetBlockedCntRatio == 2
        assert factors.MarketSubnetCatalogReqsCntRatio == 3
        assert factors.MarketSubnetEnemyCntRatio == 4
        assert factors.MarketSubnetEnemyRedirectsCntRatio == 5
        assert factors.MarketSubnetFuidCntRatio == 6
        assert factors.MarketSubnetHostingCntRatio == 7
        assert factors.MarketSubnetIcookieCntRatio == 8
        assert factors.MarketSubnetIpv4CntRatio == 9
        assert factors.MarketSubnetIpv6CntRatio == 10
        assert factors.MarketSubnetLoginCntRatio == 11
        assert factors.MarketSubnetMobileCntRatio == 12
        assert factors.MarketSubnetOtherHandlesReqsCntRatio == 13
        assert factors.MarketSubnetProductReqsCntRatio == 14
        assert factors.MarketSubnetProxyCntRatio == 15
        assert factors.MarketSubnetRefererIsEmptyCntRatio == 16
        assert factors.MarketSubnetRefererIsNotYandexCntRatio == 17
        assert factors.MarketSubnetRefererIsYandexCntRatio == 18
        assert factors.MarketSubnetRobotsCntRatio == 19
        assert factors.MarketSubnetSearchReqsCntRatio == 20
        assert factors.MarketSubnetSpravkaCntRatio == 21
        assert factors.MarketSubnetTorCntRatio == 22
        assert factors.MarketSubnetVpnCntRatio == 23
        assert factors.MarketSubnetYndxIpCntRatio == 24
