#!/skynet/bin/python

from cm_test import run_cm_test


def test_cm_workers_priority2_linux():
    run_cm_test("cm_workers_priority2_linux")
    return "Ok"
