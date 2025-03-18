OWNER(
    g:clustermaster
    g:kwyt
)

PY2TEST()

TEST_SRCS(
    unit_tests.py
    cm_cpu_and_mem_load_linux.py
    cm_cron_linux.py
    cm_cron_multihosts_linux.py
    cm_crossnodes_linux.py
    cm_cycle_linux.py
    cm_fail_states_linux.py
    cm_fetch_links_linux.py
    cm_host_dependence_linux.py
    cm_host_diff_linux.py
    cm_mapped_deepfail_linux.py
    cm_multi_cluster_dep_linux.py
    cm_multi_cross_dep_linux.py
    cm_multi_forced_linux.py
    cm_multi_host_dep_linux.py
    cm_process_limit_linux.py
    cm_recpriority_linux.py
    cm_res_redefine_linux.py
    cm_resources_linux.py
    cm_robmerge_tree_linux.py
    cm_run_branch_linux.py
    cm_shared_res_linux.py
    cm_simple_dependence_linux.py
    cm_simple_semafors_linux.py
    cm_stopped_process_linux.py
    cm_use_stop_in_merge_linux.py
    cm_use_stop_linux.py
    cm_use_stop_resources_linux.py
    cm_walrus_clusters_linux.py
    cm_worker_suicide_linux.py
    cm_workers_priority2_linux.py
    cm_workers_priority_linux.py
)

TIMEOUT(1200)

TAG(ya:not_autocheck)

DATA(arcadia/check/robotcheck)

DEPENDS(
    tools/clustermaster/master
    tools/clustermaster/worker
    tools/clustermaster/remote
    tools/clustermaster/ut
    tools/clustermaster/solver
    tools/clustermaster/tests/vbox
)



END()

RECURSE(vbox)
