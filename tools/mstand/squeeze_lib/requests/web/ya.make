LIBRARY()

OWNER(
    g:mstand
)

SRCS(
    web_request.cpp
)

PEERDIR(
    mapreduce/yt/interface
    quality/ab_testing/cost_join_lib
    quality/ab_testing/lib_calc_session_metrics/common
    quality/ab_testing/stat_collector_lib/common/logs
    quality/ab_testing/stat_collector_lib/common/url
    quality/functionality/turbo/urls_lib/cpp/lib
    quality/logs/baobab/api/cpp/common
    quality/user_metrics/common
    quality/user_metrics/vertical_metrics
    quality/user_sessions/request_aggregate_lib
    quality/webfresh/learn/parser_lib
    search/web/blender/util
    search/web/util/sources
    tools/mstand/squeeze_lib/requests/common
)

END()
