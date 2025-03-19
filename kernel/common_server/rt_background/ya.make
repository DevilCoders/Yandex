LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/library/time_restriction
    kernel/common_server/rt_background/storages
    kernel/common_server/rt_background/tasks
)

SRCS(
    GLOBAL state.cpp
    manager.cpp
    settings.cpp
    config.cpp
)

END()
