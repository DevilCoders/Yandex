PROGRAM()

OWNER(g:cloud-nbs)

ALLOCATOR(TCMALLOC_TC)

GENERATE_ENUM_SERIALIZATION(options.h)

SRCS(
    app.cpp
    bootstrap.cpp
    initiator.cpp
    main.cpp
    options.cpp
    probes.cpp
    protocol.cpp
    protocol.proto
    runnable.cpp
    storage.cpp
    storage_local_aio.cpp
    storage_local_uring.cpp
    storage_memory.cpp
    storage_null.cpp
    storage_rdma.cpp
    target.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/rdma
    cloud/storage/core/libs/diagnostics
    library/cpp/getopt
    library/cpp/lwtrace
    library/cpp/lwtrace/mon
    library/cpp/monlib/dynamic_counters
    contrib/libs/libaio
    contrib/libs/liburing
    library/cpp/deprecated/atomic
)

END()
