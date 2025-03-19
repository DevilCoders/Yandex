LIBRARY()

OWNER(ivanmorozov)

PEERDIR(
    kernel/common_server/server
    kernel/common_server/solutions/fallback_proxy/src/abstract
    kernel/common_server/solutions/fallback_proxy/src/permissions
    kernel/common_server/solutions/fallback_proxy/src/pq_message
    kernel/common_server/solutions/fallback_proxy/src/rt_background
    kernel/common_server/solutions/fallback_proxy/src/handlers
    kernel/common_server/solutions/fallback_proxy/src/filters
)

SRCS(
    server.cpp
    config.cpp
)

END()
