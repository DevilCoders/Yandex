LIBRARY()

OWNER(ivanmorozov)

PEERDIR(
    kernel/daemon/module
    library/cpp/messagebus/scheduler
    library/cpp/logger
)

SRCS(
    GLOBAL parent_existence_module.cpp
)

END()
