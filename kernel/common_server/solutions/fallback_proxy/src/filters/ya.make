LIBRARY()

OWNER(mennibaev)

PEERDIR(
    fintech/backend-kotlin/services/audit/api
    kernel/common_server/library/persistent_queue/abstract
)

SRCS(
    GLOBAL filter.cpp
    GLOBAL service_id.cpp
)

END()
