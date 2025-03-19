LIBRARY()

OWNER(g:cloud-nbs)

GENERATE_ENUM_SERIALIZATION(error.h)

SRCS(
    affinity.cpp
    alloc.cpp
    app.cpp
    byte_vector.cpp
    concurrent_queue.cpp
    error.cpp
    file_io_service.cpp
    format.cpp
    helpers.cpp
    media.cpp
    random.cpp
    scheduler.cpp
    scheduler_test.cpp
    startable.cpp
    task_queue.cpp
    thread.cpp
    thread_park.cpp
    thread_pool.cpp
    timer.cpp
    timer_test.cpp
    verify.cpp
)

PEERDIR(
    cloud/storage/core/protos
    library/cpp/actors/prof
    library/cpp/actors/util
    library/cpp/logger
    library/cpp/threading/future
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
