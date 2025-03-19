LIBRARY()

OWNER(g:cloud-nbs)

GENERATE_ENUM_SERIALIZATION(options.h)

SRCS(
    app.cpp
    bootstrap.cpp
    config_initializer.cpp
    options.cpp
)

PEERDIR(
    cloud/blockstore/libs/client
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/discovery
    cloud/blockstore/libs/endpoints
    cloud/blockstore/libs/endpoints_grpc
    cloud/blockstore/libs/endpoints_nbd
    cloud/blockstore/libs/endpoints_rdma
    cloud/blockstore/libs/endpoints_spdk
    cloud/blockstore/libs/endpoints_vhost
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/rdma
    cloud/blockstore/libs/server
    cloud/blockstore/libs/service
    cloud/blockstore/libs/service_kikimr
    cloud/blockstore/libs/service_local
    cloud/blockstore/libs/service_throttling
    cloud/blockstore/libs/spdk
    cloud/blockstore/libs/storage_local
    cloud/blockstore/libs/storage/core
    cloud/blockstore/libs/storage/init
    cloud/blockstore/libs/throttling
    cloud/blockstore/libs/validation
    cloud/blockstore/libs/vhost
    cloud/blockstore/libs/ydbstats
    cloud/storage/core/libs/common
    cloud/storage/core/libs/daemon
    cloud/storage/core/libs/diagnostics
    cloud/storage/core/libs/coroutine
    cloud/storage/core/libs/grpc
    cloud/storage/core/libs/keyring
    cloud/storage/core/libs/version
    library/cpp/actors/util
    library/cpp/getopt
    library/cpp/getopt/small
    library/cpp/logger
    library/cpp/lwtrace/mon
    library/cpp/monlib/dynamic_counters
    library/cpp/protobuf/util
    library/cpp/sighandler
    ydb/core/blobstorage/lwtrace_probes
    ydb/core/protos
    ydb/core/tablet_flat
    ydb/library/yql/public/udf/service/exception_policy
)

END()

RECURSE_FOR_TESTS(
    ut
)
