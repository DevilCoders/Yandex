# coding: utf-8

import pytest
from yatest import common


test_data = [
    [
        "-f", common.data_path("wizard/language/morphfixlist.txt"), common.source_path("tools/langdiscr-test/tests/requests.txt"),
    ],
    [
        "-m", "-f", common.data_path("wizard/language/morphfixlist.txt"), "-l", common.data_path("wizard/language/langdiscr.lst"),
        common.source_path("tools/langdiscr-test/tests/requests.txt"),
    ],
]


@pytest.mark.parametrize("args", test_data, ids=range(1, len(test_data) + 1))
def test_langdiscr(args, request):
    return common.canonical_execute(common.binary_path("tools/langdiscr-test/langdiscr-test"),
        args, check_exit_code=False, timeout=240)
