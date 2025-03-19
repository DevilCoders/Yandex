LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/tvm_services
    kernel/common_server/abstract
)

SRCS(
    agent.cpp
    GLOBAL direct.cpp
    GLOBAL template.cpp
    constructor.cpp
)

END()
