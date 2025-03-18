import json
import re

import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import asserts


class BaseTestRequestClassifierRules(AntirobotTestSuite):
    def req_type(self, request):
        output = self.req2info(request)
        return re.search("ReqType: (.*)", output).group(1)

    def req_group(self, request):
        output = self.req2info(request)
        return re.search("ReqGroup: (.*)", output).group(1)


class TestBadRequestClassifierRules(BaseTestRequestClassifierRules):
    options = {"re_queries": [{
        "path": "BAD_RULE",
        "req_type": "BAD_RULE",
    }]}

    @classmethod
    def setup_class(cls):
        try:
            super().setup_class()
            cls.fail = False
        except Exception:
            cls.fail = True

    @classmethod
    def teardown_class(cls):
        if not cls.fail:
            super().teardown_class()

    def test_fail_when_rules_are_bad(self):
        assert self.fail


class TestReloadRequestClassifierRules(BaseTestRequestClassifierRules):
    options = {
        "re_queries@market": [{
            "path": "/a",
            "req_type": "catalogoffers",
        }],
        "re_groups@market": [
            {
                "path": "/x",
                "req_group": "foobar",
            },
            {
                "path": "/y",
                "req_group": "barbaz",
            },
        ],
    }

    def xtest_handle_reload(self):
        assert self.req_type("http://market.yandex.ru/a") == "catalogoffers"

        service_config_path = self.antirobot.dump_cfg()["JsonConfFilePath"]

        with open(service_config_path) as service_config_file:
            service_config = json.load(service_config_file)

        for service in service_config:
            if service["service"] == "market":
                service["re_queries"] = [{
                    "path": "/a",
                    "req_type": "modelprices",
                }]

                break

        with open(service_config_path, "w") as service_config_file:
            json.dump(service_config, service_config_file)

        self.antirobot.reload_data()

        assert self.antirobot.is_alive()
        asserts.AssertEventuallyTrue(
            lambda: self.req_type("http://market.yandex.ru/a") == "modelprices",
        )

    @pytest.mark.parametrize("path, group", (
        ("/aaa", "generic"),
        ("/search/itditp/?filter_viewed", "generic"),
        ("/search/xxx/?filter_viewed", "search"),
        ("/search/?text=qwe", "search"),
        ("/suggest/suggest-browser?a=1", "generic"),
    ))
    def test_req_groups(self, path, group):
        req_url = f"http://yandex.ru/{path}"
        assert self.req_group(req_url) == group
        some_other_group = "search" if group == "generic" else "generic"

        old_true_value = self.antirobot.query_metric("requests_passed_to_service_req_deee", req_group=group)
        old_false_value = self.antirobot.query_metric("requests_passed_to_service_req_deee", req_group=some_other_group)
        self.send_fullreq(req_url)
        new_true_value = self.antirobot.query_metric("requests_passed_to_service_req_deee", req_group=group)
        new_false_value = self.antirobot.query_metric("requests_passed_to_service_req_deee", req_group=some_other_group)
        assert new_true_value - old_true_value == 1
        assert new_false_value == old_false_value
