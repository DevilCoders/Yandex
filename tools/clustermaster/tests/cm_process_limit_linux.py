#!/skynet/bin/python

from cm_test import run_cm_test


def test_cm_process_limit_linux():
    run_cm_test("cm_process_limit_linux")
    return "Ok"
