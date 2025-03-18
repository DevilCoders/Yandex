#!/skynet/bin/python

from cm_test import run_cm_test


def test_cm_cron_linux():
    run_cm_test("cm_cron_linux")
    return "Ok"
