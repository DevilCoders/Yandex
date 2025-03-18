#!/skynet/bin/python

from cm_test import run_cm_test


def test_cm_workers_priority_linux():
    run_cm_test("cm_workers_priority_linux")
    return "Ok"
