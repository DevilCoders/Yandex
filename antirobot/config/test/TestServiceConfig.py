import yatest
import pytest
import json


class TestServiceConfig:
    @classmethod
    def setup_class(cls):
        with open(yatest.common.source_path("antirobot/config/service_config.json")) as inp:
            cls.service_config = json.load(inp)

    def get_flags(self, config, flag):
        value = config[flag]
        if type(value) == int:
            return [value]
        assert type(value) == list
        return value

    @pytest.mark.parametrize("mandatory_key, mandatory_value, exceptions", [
        ("cbb_re_flag", 183, []),
        ("cbb_re_mark_flag", 185, []),
        ("cbb_re_mark_log_only_flag", 702, []),
        ("cbb_captcha_re_flag", 262, []),
        ("cbb_ip_flag", 162, ["music", "kinopoisk"]),
        ("cbb_farmable_ban_flag", 226, []),
    ])
    def test_mandatory_flags(self, mandatory_key, mandatory_value, exceptions):
        for cfg in self.service_config:
            flag_value = self.get_flags(cfg, mandatory_key)
            service = cfg["service"]
            if service not in exceptions:
                assert mandatory_value in flag_value, f"Check {mandatory_key} for {service}"
