OWNER(g:clustermaster)

LIBRARY()

# FixMe : linking to devstat lib
IF (OS_FREEBSD)
    EXTRALIBS(-ldevstat)
ENDIF()

ADDINCL(tools/clustermaster/master)

SRCDIR(tools/clustermaster/worker)

SRCS(
    http.cpp
    diskspace_notifier.cpp
    modstate.cpp
    resource_manager.cpp
    resource_monitor.cpp
    semaphore.cpp
    worker.cpp
    worker_depend.cpp
    worker_list_manager.cpp
    worker_target.cpp
    worker_target_graph.cpp
    worker_target_type.cpp
    worker_workflow.cpp
)

PEERDIR(
    ADDINCL tools/clustermaster/common
    ADDINCL tools/clustermaster/libremote
    library/cpp/getoptpb
    library/cpp/http/server
    library/cpp/protobuf/util
    library/cpp/svnversion
    library/cpp/terminate_handler
    tools/clustermaster/proto
    tools/clustermaster/communism/client
    tools/clustermaster/communism/util
)

END()
