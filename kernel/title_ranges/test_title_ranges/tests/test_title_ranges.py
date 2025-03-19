# coding: utf-8

from yatest import common


def test_test_title_ranges():
    return common.canonical_execute(common.binary_path("kernel/title_ranges/test_title_ranges/test_title_ranges"),
        [common.source_path("kernel/title_ranges/test_title_ranges/tests/input")], check_exit_code=False, timeout=20)
