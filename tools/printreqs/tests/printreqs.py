# coding: utf-8

import pytest
from yatest import common


test_data = [
    [
        common.source_path("tools/printreqs/tests/test_req.txt"),
    ],
    [
        "-x", common.source_path("tools/printreqs/tests/test_req.txt"),
    ],
]


@pytest.mark.parametrize("args", test_data, ids=map(str, range(1, len(test_data) + 1)))
def test_printreqs(args, request):
    return common.canonical_execute(common.binary_path("tools/printreqs/printreqs"),
        args, check_exit_code=False, timeout=40)
