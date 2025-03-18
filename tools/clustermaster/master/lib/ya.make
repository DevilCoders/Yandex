OWNER(g:clustermaster)

LIBRARY()

SRCDIR(tools/clustermaster/master)

SRCS(
    http.cpp
    http_clusters.cpp
    http_command.cpp
    http_common.cpp
    http_graph.cpp
    http_proxy.cpp
    http_summary.cpp
    http_targets.cpp
    http_variables.cpp
    http_workers.cpp
    master.cpp
    master_config.cpp
    master_depend.cpp
    master_target.cpp
    master_target_graph.cpp
    master_target_type.cpp
    notification.cpp
    cron_state.cpp
    worker_pool.cpp
    target_stats.cpp
)

PEERDIR(
    ADDINCL tools/clustermaster/common
    devtools/yanotifybot/client/cpp
    library/cpp/cache
    library/cpp/deprecated/transgene
    library/cpp/digest/old_crc
    library/cpp/getoptpb
    library/cpp/http/server
    library/cpp/http/simple
    library/cpp/json
    library/cpp/json/writer
    library/cpp/svnversion
    library/cpp/terminate_handler
    library/cpp/tvmauth/client
    library/cpp/xml/encode
    library/cpp/xsltransform
    tools/clustermaster/common
    tools/clustermaster/proto
    tools/clustermaster/communism/util
    yweb/config
)

END()
