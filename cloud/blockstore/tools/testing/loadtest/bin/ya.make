PROGRAM(blockstore-loadtest)

OWNER(g:cloud-nbs)

ALLOCATOR(TCMALLOC_TC)

SRCS(
    app.cpp
    bootstrap.cpp
    main.cpp
    options.cpp
)

PEERDIR(
    cloud/blockstore/public/api/protos

    cloud/blockstore/tools/testing/loadtest/lib
    cloud/blockstore/tools/testing/loadtest/protos

    cloud/blockstore/libs/client
    cloud/blockstore/libs/client_rdma
    cloud/blockstore/libs/client_spdk
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/encryption
    cloud/blockstore/libs/nbd
    cloud/blockstore/libs/rdma
    cloud/blockstore/libs/spdk

    cloud/storage/core/libs/grpc
    cloud/storage/core/libs/version

    library/cpp/deprecated/atomic
    library/cpp/getopt
    library/cpp/logger
    library/cpp/protobuf/json
    library/cpp/protobuf/util
    library/cpp/sighandler
)

END()
