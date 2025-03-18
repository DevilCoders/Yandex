# coding: utf-8

from yatest import common


def test_file_checker():
    return common.canonical_execute(common.binary_path("library/cpp/file_checker/test/file_checker_test"),
        [common.output_path("test"), "10", "10", "20", "2000"], check_exit_code=False, timeout=180)
