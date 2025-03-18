#!/skynet/bin/python

from cm_test import run_cm_test


def test_cm_cpu_and_mem_load_linux():
    run_cm_test("cm_cpu_and_mem_load_linux")
    return "Ok"
