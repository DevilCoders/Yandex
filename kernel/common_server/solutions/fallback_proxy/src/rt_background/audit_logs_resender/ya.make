LIBRARY()

OWNER(mennibaev)

SRCS(
    GLOBAL resender.cpp
)

PEERDIR(
    fintech/backend-kotlin/services/audit/api
    kernel/common_server/solutions/fallback_proxy/src/rt_background/queue_resender
)

END()
