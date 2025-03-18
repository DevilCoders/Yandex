#!/skynet/bin/python

from cm_test import run_cm_test


def test_cm_stopped_process_linux():
    run_cm_test("cm_stopped_process_linux")
    return "Ok"
