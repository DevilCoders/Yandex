UNITTEST()

SRCDIR(cloud/storage/core/libs/common)

PEERDIR(
    cloud/storage/core/libs/common
)

OWNER(g:cloud-nbs)

SRCS(
    concurrent_queue_ut.cpp
    error_ut.cpp
    file_io_service_ut.cpp
    scheduler_ut.cpp
    thread_pool_ut.cpp
)

END()
