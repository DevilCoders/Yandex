import base64
import calendar
import datetime
import itertools
import json
import os

import jwt
import pytest
import yatest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite, TEST_DATA_ROOT

from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    GenRandomIP,
)


IP_HEADER = 'X-Forwarded-For-Y'
REGULAR_SEARCH = 'http://yandex.ru/search?text=cats'
RPS_FACTOR_NAME = 'requests_per_second'


class TestAntirobotFactors(AntirobotTestSuite):
    options = {
        "PartOfAllFactorsToPrint": 1.0,
        "EventLogFrameSize": 0
    }

    global_config={
        "rules": [
            {"id": 1, "cbb": [], "yql": ["has_valid_market_jws == 1"]},
            {"id": 2, "cbb": [], "yql": ["has_valid_market_jws == 0"]},
        ],
    }

    @staticmethod
    def get_latest_factors_file():
        factors_versions_dir = yatest.common.source_path("antirobot/daemon_lib/factors_versions")
        last_path = None
        for check_for_version in itertools.count(55):
            test_path = os.path.join(factors_versions_dir, f"factors_{check_for_version}.inc")
            if not os.path.exists(test_path):
                assert check_for_version >= 57
                return last_path
            last_path = test_path

    def get_factor_position(self, factor_name):
        with open(self.get_latest_factors_file(), "r") as inp:
            factors = inp.readlines()
            return factors.index('"%s",\n' % factor_name)

    def test_rps_factor_logged(self):
        request = Fullreq(REGULAR_SEARCH, headers={IP_HEADER: GenRandomIP()})

        self.antirobot.send_request(request)

        rps_entries_event = self.unified_agent.wait_event_logs(["TAntirobotFactors"])[0]

        rps_factor_position = self.get_factor_position(RPS_FACTOR_NAME)
        rps_factor = rps_entries_event.Event.Factors[rps_factor_position]

        rps_after_single_entry = 1.0 / 60
        eps = 1e-6
        assert abs(rps_factor - rps_after_single_entry) <= eps

    @pytest.mark.parametrize("tamper", [True, False])
    def test_market_jws_factor(self, tamper):
        with open(TEST_DATA_ROOT / "data" / "market_jws_key") as key_file:
            key = base64.b64decode(key_file.read().strip())

        now = calendar.timegm(datetime.datetime.utcnow().utctimetuple())
        token = jwt.encode({"iat": now, "exp": now + 3600}, key, algorithm="HS256")

        if tamper:
            fake_payload_bytes = json.dumps({"iat": now + 1, "exp": now + 3601}).encode()
            fake_payload = base64.b64encode(fake_payload_bytes).decode()

            payload_start = token.index(".") + 1
            payload_end = token.index(".", payload_start)
            token = token[:payload_start] + fake_payload + token[payload_end:]

        self.antirobot.ping_search(headers={"X-Jws": token})
        rules = self.unified_agent.get_last_event_in_daemon_logs().get("rules", [])

        if tamper:
            assert rules == ["2"]
        else:
            assert rules == ["1"]
