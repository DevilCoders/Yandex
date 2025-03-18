# coding: utf-8

from yatest import common


def test_nodeiter_test():
    with open(common.source_path("tools/nodeiter_test/tests/test_req.txt")) as stdin:
        return common.canonical_execute(common.binary_path("tools/nodeiter_test/nodeiter_test"),
            [], stdin=stdin,
            check_exit_code=False, timeout=80)
