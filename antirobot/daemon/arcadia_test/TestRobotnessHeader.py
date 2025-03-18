import time

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util.asserts import AssertEventuallyTrue
from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    IsBlockedResponse,
    IsCaptchaRedirect,
    ICOOKIE_HEADER,
    IP_HEADER,
    VALID_I_COOKIE
)


ROBOTNESS_HEADER = "X-Antirobot-Robotness-Y"
CBB_FLAG_BAN_BY_IP = 13
CBB_FLAG_BAN_BY_REGEXP = 14
CBB_FLAG_BLOCK_BY_IP = 15
CBB_FLAG_BLOCK_BY_IP_SECONDARY = 17
CBB_FLAG_BLOCK_BY_REGEXP = 16
CBB_FLAG_MAX_ROBOTNESS = 329

REGULAR_SEARCH = "http://yandex.ru/search?text=123"
ROBOT_SEARCH = "http://yandex.ru/search?text=test"


def search(q):
    return f"http://yandex.ru/search?text=last_req_{q}"


class TestRobotnessHeader(AntirobotTestSuite):
    options = {
        "CbbSyncPeriod": "1s",
        "cbb_captcha_re_flag": CBB_FLAG_BAN_BY_REGEXP,
        "CbbFlagIpBasedIdentificationsBan": CBB_FLAG_BAN_BY_IP,
        "CbbFlagMaxRobotness": CBB_FLAG_MAX_ROBOTNESS,
        "cbb_ip_flag": [CBB_FLAG_BLOCK_BY_IP, CBB_FLAG_BLOCK_BY_IP_SECONDARY],
        "cbb_re_flag": CBB_FLAG_BLOCK_BY_REGEXP,
        "DisableBansByFactors": 1,
    }
    num_old_antirobots = 0
    captcha_args = ["--generate", "correct"]

    def test_regular(self):
        ip = '2.1.2.1'

        request = Fullreq(search(ip), headers={IP_HEADER: ip})
        response = self.send_request(request)

        assert not IsCaptchaRedirect(response)
        assert not IsBlockedResponse(response)
        assert response.headers[ROBOTNESS_HEADER] == "0.0"
        self.check_last_robotness_header(ip, "0")

    def test_cannot_show_captcha(self):
        ip = '2.1.2.2'
        self.antirobot.ban(ip)

        request = Fullreq(
            "http://yandex.ru/nonexistent", headers={IP_HEADER: ip}
        )
        response = self.send_request(request)

        assert not IsCaptchaRedirect(response)
        assert not IsBlockedResponse(response)
        assert response.headers[ROBOTNESS_HEADER] == "1.0"

    def test_banned(self):
        ip = '2.1.2.3'
        self.antirobot.ban(ip)

        request = Fullreq(REGULAR_SEARCH, headers={IP_HEADER: ip})
        response = self.send_request(request)

        assert IsCaptchaRedirect(response)
        assert ROBOTNESS_HEADER not in response.headers  # ответ To-User, хедера быть не должно

    def test_blocked(self):
        ip = '2.1.2.4'
        self.antirobot.block(ip)

        request = Fullreq(search(ip), headers={IP_HEADER: ip})
        response = self.send_request(request)

        assert IsBlockedResponse(response)
        assert ROBOTNESS_HEADER not in response.headers  # ответ To-User, хедера быть не должно
        self.check_last_robotness_header(ip, "1")

    def check_last_robotness_header(self, query, expected_header_value):
        row = self.unified_agent.wait_log_line_with_query(f".*text=last_req_{query}.*")
        assert row["robotness"] == float(expected_header_value)
        cacher_log = self.unified_agent.wait_cacher_log_line_with_query(f".*text=last_req_{query}.*")
        assert cacher_log.robotness == float(expected_header_value)

    def test_robotness_header_from_cbb_ban(self):
        ip = '2.1.2.5'
        request = Fullreq(
            ROBOT_SEARCH,
            headers={
                IP_HEADER: ip,
                ICOOKIE_HEADER: VALID_I_COOKIE,
            }
        )

        self.cbb.add_text_block(CBB_FLAG_BAN_BY_REGEXP, "cgi=/.*test.*/")

        AssertEventuallyTrue(
            lambda: IsCaptchaRedirect(self.send_request(request))
        )
        time.sleep(2)

        request = Fullreq(
            search(ip),
            headers={
                IP_HEADER: ip,
                ICOOKIE_HEADER: VALID_I_COOKIE,
            }
        )
        assert ROBOTNESS_HEADER not in self.send_request(request).headers  # ответ To-User, хедера быть не должно
        self.check_last_robotness_header(ip, "1")

    def test_mark_max_robotness_by_cbb(self):
        self.cbb.add_text_block(CBB_FLAG_MAX_ROBOTNESS, "cgi=/.*max_robotness.*/")

        ip = '2.1.2.6'
        request = Fullreq(
            "http://yandex.ru/search?text=max_robotness",
            headers={IP_HEADER: ip},
        )

        AssertEventuallyTrue(
            lambda: self.send_request(request).headers[ROBOTNESS_HEADER] == "1.0"
        )
        # header contains 1.0 because of CBB flag, not because of ban
        assert not IsCaptchaRedirect(self.send_request(request))
