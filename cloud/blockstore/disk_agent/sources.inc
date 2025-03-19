SRCS(
    app.cpp
    bootstrap.cpp
    config_initializer.cpp
    main.cpp
    options.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/server
    cloud/blockstore/libs/service
    cloud/blockstore/libs/spdk
    cloud/blockstore/libs/storage/core
    cloud/blockstore/libs/storage/init

    cloud/storage/core/libs/daemon
    cloud/storage/core/libs/diagnostics
    cloud/storage/core/libs/version

    ydb/core/protos
    library/cpp/lwtrace/mon

    library/cpp/actors/util
    library/cpp/getopt
    library/cpp/getopt/small
    library/cpp/logger
    library/cpp/monlib/dynamic_counters
    library/cpp/protobuf/util
    library/cpp/sighandler

    ydb/library/yql/public/udf/service/exception_policy
)

IF (BUILD_TYPE != "PROFILE")
    SPLIT_DWARF()
ENDIF()
