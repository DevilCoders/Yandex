OWNER(
    g:clustermaster
    g:kwyt
)

UNITTEST()

PEERDIR(
    ADDINCL tools/clustermaster/common
    ADDINCL tools/clustermaster/master/lib
    ADDINCL tools/clustermaster/worker/lib
    tools/clustermaster/master/graph-test
    tools/clustermaster/worker/graph-test
)

IF (OS_FREEBSD)
    EXTRALIBS(-ldevstat)
ENDIF()

SRCDIR(${ARCADIA_ROOT})

SRCS(
    tools/clustermaster/common/cron_ut.cpp
    tools/clustermaster/common/cross_enumerator_ut.cpp
    tools/clustermaster/common/id_for_string_ut.cpp
    tools/clustermaster/common/master_list_manager_ut.cpp
    tools/clustermaster/common/param_mapping_ut.cpp
    tools/clustermaster/common/param_mapping_detect_ut.cpp
    tools/clustermaster/common/param_sort_ut.cpp
    tools/clustermaster/common/state_registry_ut.cpp
    tools/clustermaster/common/target_graph_parser_ut.cpp
    tools/clustermaster/common/target_graph_ut.cpp
    tools/clustermaster/common/target_type_parameters_ut.cpp
    tools/clustermaster/common/worker_variables_ut.cpp
    tools/clustermaster/common/util_ut.cpp
    tools/clustermaster/master/graph-test/master_target_graph_ut.cpp
    tools/clustermaster/master/time_counters_ut.cpp
    tools/clustermaster/worker/graph-test/worker_target_graph_ut.cpp
)

END()
