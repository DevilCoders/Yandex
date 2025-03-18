#!/skynet/bin/python

from yatest.common import execute, canonical_execute, binary_path, source_path, work_path, canonical_file


def run_cm_test(testname):
    execute(["{}/run_test.sh".format(source_path("tools/clustermaster/tests")),
        testname, source_path(""), binary_path(""), work_path("")])


def test_unit_tests():
    return canonical_execute(binary_path("tools/clustermaster/ut/tools-clustermaster-ut"), file_name="tools-clustermaster-ut")

def test_cm_cpu_and_mem_load_linux():
    run_cm_test("cm_cpu_and_mem_load_linux")
    return "Ok"

def test_cm_cron_linux():
    run_cm_test("cm_cron_linux")
    return "Ok"

def test_cm_cron_multihosts_linux():
    run_cm_test("cm_cron_multihosts_linux")
    return "Ok"

def test_cm_crossnodes_linux():
    run_cm_test("cm_crossnodes_linux")
    return "Ok"

def test_cm_cycle_linux():
    run_cm_test("cm_cycle_linux")
    return "Ok"

def test_cm_fail_states_linux():
    run_cm_test("cm_fail_states_linux")
    return "Ok"

def test_cm_fetch_links_linux():
    run_cm_test("cm_fetch_links_linux")
    return "Ok"

def test_cm_host_dependence_linux():
    run_cm_test("cm_host_dependence_linux")
    return "Ok"

def test_cm_host_diff_linux():
    run_cm_test("cm_host_diff_linux")
    return "Ok"

def test_cm_mapped_deepfail_linux():
    run_cm_test("cm_mapped_deepfail_linux")
    return "Ok"

def test_cm_multi_cluster_dep_linux():
    run_cm_test("cm_multi_cluster_dep_linux")
    return "Ok"

def test_cm_multi_cross_dep_linux():
    run_cm_test("cm_multi_cross_dep_linux")
    return "Ok"

def test_cm_multi_forced_linux():
    run_cm_test("cm_multi_forced_linux")
    return "Ok"

def test_cm_multi_host_dep_linux():
    run_cm_test("cm_multi_host_dep_linux")
    return "Ok"

def test_cm_process_limit_linux():
    run_cm_test("cm_process_limit_linux")
    return "Ok"

def test_cm_recpriority_linux():
    run_cm_test("cm_resources_linux")
    return "Ok"

def test_cm_res_redefine_linux():
    run_cm_test("cm_res_redefine_linux")
    return "Ok"

def test_cm_resources_linux():
    run_cm_test("cm_resources_linux")
    return "Ok"

def test_cm_robmerge_tree_linux():
    run_cm_test("cm_use_stop_resources_linux")
    return "Ok"

def test_cm_run_branch_linux():
    run_cm_test("cm_run_branch_linux")
    return "Ok"

def test_cm_shared_res_linux():
    run_cm_test("cm_shared_res_linux")
    return "Ok"

def test_cm_simple_dependence_linux():
    run_cm_test("cm_simple_dependence_linux")
    return "Ok"

def test_cm_simple_semafors_linux():
    run_cm_test("cm_simple_semafors_linux")
    return "Ok"

def test_cm_stopped_process_linux():
    run_cm_test("cm_stopped_process_linux")
    return "Ok"

def test_cm_use_stop_in_merge_linux():
    run_cm_test("cm_use_stop_in_merge_linux")
    return "Ok"

def test_cm_use_stop_linux():
    run_cm_test("cm_use_stop_linux")
    return "Ok"

def test_cm_use_stop_resources_linux():
    run_cm_test("cm_use_stop_resources_linux")
    return "Ok"

def test_cm_walrus_clusters_linux():
    run_cm_test("cm_walrus_clusters_linux")
    return "Ok"

def test_cm_worker_suicide_linux():
    run_cm_test("cm_worker_suicide_linux")
    return "Ok"

def test_cm_workers_priority2_linux():
    run_cm_test("cm_workers_priority2_linux")
    return "Ok"

def test_cm_workers_priority_linux():
    run_cm_test("cm_workers_priority_linux")
    return "Ok"
