LIBRARY()

OWNER(
    g:mstand
)

SRCS(
    base_request.cpp
    field_extractor.cpp
)

PEERDIR(
    quality/ab_testing/lib_calc_session_metrics/common
    quality/ab_testing/stat_collector_lib/common/logs
    quality/logs/baobab/api/cpp/common
    quality/user_sessions/request_aggregate_lib
)

GENERATE_ENUM_SERIALIZATION(base_request.h)

END()
