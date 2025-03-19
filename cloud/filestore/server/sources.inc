IF (BUILD_TYPE != "PROFILE" && BUILD_TYPE != "DEBUG")
    SPLIT_DWARF()
ENDIF()

SRCS(
    app.cpp
    bootstrap.cpp
    main.cpp
    options.cpp
)

PEERDIR(
    cloud/filestore/libs/daemon
    cloud/filestore/libs/diagnostics
    cloud/filestore/libs/server
    cloud/filestore/libs/service_kikimr
    cloud/filestore/libs/service_local
    cloud/filestore/libs/service_null
    cloud/filestore/libs/storage/init

    cloud/storage/core/libs/common
    cloud/storage/core/libs/daemon
    cloud/storage/core/libs/diagnostics
    cloud/storage/core/libs/kikimr
    cloud/storage/core/libs/version

    kikimr/yndx/keys

    library/cpp/actors/util
    library/cpp/getopt
    library/cpp/logger
    library/cpp/lwtrace/mon
    library/cpp/protobuf/util
    library/cpp/sighandler

    ydb/core/protos
    ydb/library/yql/public/udf/service/exception_policy
    ydb/public/sdk/cpp/client/ydb_persqueue_public/codecs
)
