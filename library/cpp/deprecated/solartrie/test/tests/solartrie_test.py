# coding: utf-8

from yatest import common


def test_solartrie():
    with open(common.data_path("solartrie_tests_data/input.txt.gz")) as stdin:
        return common.canonical_execute(common.binary_path("library/cpp/deprecated/solartrie/test/solartrie_test"),
            [], stdin=stdin,
            check_exit_code=False, timeout=180)
