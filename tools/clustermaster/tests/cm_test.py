#!/skynet/bin/python

from yatest.common import execute, canonical_execute, binary_path, source_path, work_path, canonical_file


def run_cm_test(testname):
    execute(["{}/run_test.sh".format(source_path("tools/clustermaster/tests")),
        testname, source_path(""), binary_path(""), work_path("")])
