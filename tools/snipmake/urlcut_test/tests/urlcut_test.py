# coding: utf-8

from yatest import common


def test_urlcut():
    with open(common.data_path("snippets_tests_data/urls.gz")) as stdin:
        return common.canonical_execute(common.binary_path("tools/snipmake/urlcut_test/urlcut_test"),
            [], stdin=stdin,
            check_exit_code=False, timeout=180)
