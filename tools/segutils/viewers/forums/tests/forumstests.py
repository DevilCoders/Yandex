# coding: utf-8

from yatest import common


def test_forums():
    return common.canonical_execute(
        common.binary_path("tools/segutils/viewers/forums/forums"),
        [
            "-f", common.data_path("forums_tests_data/files"), "-m", common.data_path("forums_tests_data/list.txt"),
            "-c", common.data_path("recognize"),
        ], check_exit_code=False, timeout=220)
