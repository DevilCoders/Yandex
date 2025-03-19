LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/settings/abstract
)

SRCS(
    abstract.cpp
    default.cpp
    GLOBAL no_balancing.cpp
    GLOBAL time_priority.cpp
    GLOBAL time_weight.cpp
    GLOBAL configured.cpp
)

END()
