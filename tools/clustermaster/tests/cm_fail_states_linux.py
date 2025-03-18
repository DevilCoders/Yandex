#!/skynet/bin/python

from cm_test import run_cm_test


def test_cm_fail_states_linux():
    run_cm_test("cm_fail_states_linux")
    return "Ok"
