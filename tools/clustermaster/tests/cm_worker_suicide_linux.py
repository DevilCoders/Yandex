#!/skynet/bin/python

from cm_test import run_cm_test


def test_cm_worker_suicide_linux():
    run_cm_test("cm_worker_suicide_linux")
    return "Ok"
