#!/skynet/bin/python

from cm_test import run_cm_test


def test_cm_recpriority_linux():
    run_cm_test("cm_recpriority_linux")
    return "Ok"
