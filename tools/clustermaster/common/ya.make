LIBRARY()

OWNER(g:clustermaster)

SRCS(
    async_jobs.cpp
    async_jobs_pool.cpp
    cgroups.cpp
    command_alias.cpp
    condition.cpp
    control_workflow.cpp
    cron.rl6
    cross_enumerator.cpp
    expand_params.cpp
    http_static.cpp
    hw_resources.cpp
    id_for_string.cpp
    listening_thread.cpp
    log.cpp
    master_list_manager.cpp
    messages.cpp
    param_list_manager.cpp
    param_mapping.cpp
    param_mapping_detect.cpp
    param_sort.cpp
    parsing_helpers.cc
    precomputed_task_ids.cpp
    precomputed_task_ids_one_side.cpp
    sockaddr.cpp
    state_file.cpp
    state_registry.cpp
    target_graph_parser.cpp
    target_graph_parser_rl.cpp.rl
    target_type.cpp
    target_type_parameters.cpp
    target_type_parameters_checksum.cpp
    thread_util.cpp
    transf_file.cpp
    util.cpp
    worker_variables.cpp
)

ARCHIVE(
    NAME http_static.inc
    http_static/style.css
    http_static/jquery-2.1.1.js
    http_static/env.js
    http_static/updater.js
    http_static/mouse.js
    http_static/hint.js
    http_static/menu.js
    http_static/summary.js
    http_static/workers.js
    http_static/targets.js
    http_static/clusters.js
    http_static/graph.js
    http_static/variables.js
    http_static/monitoring.js
    http_static/sorter.js
    http_static/graph.xslt
    http_static/favicon.png
)

PEERDIR(
    library/cpp/any
    library/cpp/archive
    library/cpp/containers/sorted_vector
    library/cpp/deprecated/fgood
    library/cpp/deprecated/split
    library/cpp/deprecated/transgene
    library/cpp/digest/md5
    library/cpp/http/server
    library/cpp/logger
    mapreduce/yt/interface/protos
    kernel/yt/dynamic
    tools/clustermaster/proto
    yweb/config
)

SET(IDE_FOLDER "_Builders")
GENERATE_ENUM_SERIALIZATION(log.h)

END()
