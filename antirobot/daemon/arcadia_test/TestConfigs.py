import json
import pytest

import yatest.common


class TestJson:
    @pytest.mark.parametrize('file', [
        "global_config.json",
        "header_hashes.json",
        "service_config.json",
        "service_identifier.json",
    ])
    def test_json_config(self, file):
        config_path = yatest.common.source_path("antirobot/config/" + file)
        with open(config_path) as f:
            json.load(f)
