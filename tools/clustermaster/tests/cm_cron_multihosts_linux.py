#!/skynet/bin/python

from cm_test import run_cm_test


def test_cm_cron_multihosts_linux():
    run_cm_test("cm_cron_multihosts_linux")
    return "Ok"
