LIBRARY()

OWNER(svshevtsov)

GENERATE_ENUM_SERIALIZATION(async_task_executor.h)

SRCS(
    async_task_executor.cpp
)

PEERDIR(
    kernel/daemon/common
    library/cpp/cgiparam
    library/cpp/json
    library/cpp/logger/global
)

END()
