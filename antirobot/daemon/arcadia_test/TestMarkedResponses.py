import time

from antirobot.daemon.arcadia_test.util import GenRandomIP
from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite

IP_HEADER = "X-Forwarded-For-Y"
CBB_MARK_FLAG_WEB = 8
CBB_MARK_FLAG_MARKET = 9
CBB_USER_MARK_FLAG_WEB = 10
CBB_USER_MARK_FLAG_MARKET = 11
CBB_SYNC_PERIOD = 1
CBB_FLAG_DEGRADATION = 661
CBB_BAN = 777


class TestMarkedResponses(AntirobotTestSuite):
    options = {
        "cbb_re_mark_flag@web": CBB_MARK_FLAG_WEB,
        "cbb_re_mark_flag@market": CBB_MARK_FLAG_MARKET,
        "cbb_re_user_mark_flag@web": CBB_USER_MARK_FLAG_WEB,
        "cbb_re_user_mark_flag@market": CBB_USER_MARK_FLAG_MARKET,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbFlagDegradation": CBB_FLAG_DEGRADATION,
        "cbb_captcha_re_flag": CBB_BAN,
    }

    captcha_args = ["--generate", "correct"]

    def test_mark_stat(self):
        self.cbb.add_text_block(CBB_MARK_FLAG_WEB, "cgi=/.*block_web.*/")
        self.cbb.add_text_block(CBB_MARK_FLAG_MARKET, "cgi=/.*block_market.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        ip = GenRandomIP()

        self.send_fullreq(
            "http://yandex.ru/search?query=block_web",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_marked_total_deee", service_type="web") == 1
        assert self.antirobot.query_metric("block_responses_marked_total_deee", service_type="market") == 0

        self.send_fullreq(
            "http://market.yandex.ru/search?query=block_market",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_marked_total_deee", service_type="web") == 1
        assert self.antirobot.query_metric("block_responses_marked_total_deee", service_type="market") == 1

    def test_user_mark_stat(self):
        self.cbb.add_text_block(CBB_USER_MARK_FLAG_WEB, "cgi=/.*block_web.*/")
        self.cbb.add_text_block(CBB_USER_MARK_FLAG_MARKET, "cgi=/.*block_market.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        ip = GenRandomIP()

        self.send_fullreq(
            "http://yandex.ru/search?query=block_web",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_user_marked_total_deee", service_type="web") == 1
        assert self.antirobot.query_metric("block_responses_user_marked_total_deee", service_type="market") == 0

        self.send_fullreq(
            "http://market.yandex.ru/search?query=block_market",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_user_marked_total_deee", service_type="web") == 1
        assert self.antirobot.query_metric("block_responses_user_marked_total_deee", service_type="market") == 1

    def test_mark_stat_blocked(self):
        self.cbb.add_text_block(CBB_MARK_FLAG_WEB, "cgi=/.*blocked_web.*/")
        self.cbb.add_text_block(CBB_MARK_FLAG_MARKET, "cgi=/.*blocked_market.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        ip = GenRandomIP()

        self.send_fullreq(
            "http://yandex.ru/search?query=blocked_web",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_marked_already_blocked_deee", service_type="web") == 0
        assert self.antirobot.query_metric("block_responses_marked_already_blocked_deee", service_type="market") == 0

        self.antirobot.block(ip)

        self.send_fullreq(
            "http://yandex.ru/search?query=blocked_web",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_marked_already_blocked_deee", service_type="web") == 1
        assert self.antirobot.query_metric("block_responses_marked_already_blocked_deee", service_type="market") == 0

        self.send_fullreq(
            "http://market.yandex.ru/search?query=blocked_market",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_marked_already_blocked_deee", service_type="web") == 1
        assert self.antirobot.query_metric("block_responses_marked_already_blocked_deee", service_type="market") == 1

    def test_mark_stat_degradation(self):
        self.cbb.add_text_block(CBB_MARK_FLAG_WEB, "cgi=/.*degradead_web.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)
        ip = GenRandomIP()

        metric = self.antirobot.query_metric("block_responses_marked_with_degradation_deee", service_type="web")

        self.send_fullreq(
            "http://yandex.ru/search?query=degradead_web",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_marked_with_degradation_deee", service_type="web") == metric

        self.cbb.add_text_block(
            CBB_FLAG_DEGRADATION, "cgi=/.*degradead_web.*/"
        )
        time.sleep(CBB_SYNC_PERIOD + 1)

        self.send_fullreq(
            "http://yandex.ru/search?query=degradead_web",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_marked_with_degradation_deee", service_type="web") == metric + 1

    def test_mark_stat_banned(self):
        self.cbb.add_text_block(CBB_MARK_FLAG_WEB, "cgi=/.*banned_web.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)
        ip = GenRandomIP()

        metric = self.antirobot.query_metric("block_responses_marked_already_banned_deee", service_type="web")

        self.send_fullreq(
            "http://yandex.ru/search?query=banned_web",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_marked_already_banned_deee", service_type="web") == metric

        self.cbb.add_text_block(CBB_BAN, "cgi=/.*banned_web.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        self.send_fullreq(
            "http://yandex.ru/search?query=banned_web",
            headers={IP_HEADER: ip},
        )

        assert self.antirobot.query_metric("block_responses_marked_already_banned_deee", service_type="web") == metric + 1
