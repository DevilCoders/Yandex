# coding: utf-8

import pytest
from yatest import common


test_data = [
    ([
        "yandex",
    ], common.source_path("tools/recode/tests/in.txt")),
    ([
        "cp1250",
    ], common.source_path("tools/recode/tests/chinese.txt")),
]


class TestRecode(object):

    def setup_method(self, method):
        self.out_path = common.output_path(method.__name__ + ".log")

    @pytest.mark.parametrize("args, data_path", test_data, ids=map(str, range(1, len(test_data) + 1)))
    def test(self, args, data_path):
        with open(data_path) as stdin:
            return common.canonical_execute(common.binary_path("tools/recode/recode"),
                ["utf8"] + args, stdin=stdin,
                check_exit_code=False, timeout=120, file_name=self.out_path)
