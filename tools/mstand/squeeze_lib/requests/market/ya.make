LIBRARY()

OWNER(
    g:mstand
)

SRCS(
    market_request.cpp
)

PEERDIR(
    mapreduce/yt/interface
    quality/ab_testing/stat_collector_lib/common/logs
    quality/ab_testing/stat_collector_lib/common/url
    quality/logs/baobab/api/cpp/common
    quality/user_metrics/common
    quality/user_metrics/vertical_metrics
    quality/user_sessions/request_aggregate_lib
    quality/webfresh/learn/parser_lib
    scarab/api/cpp
    tools/mstand/squeeze_lib/requests/common
)

END()
