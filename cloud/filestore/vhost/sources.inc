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
    cloud/filestore/libs/client
    cloud/filestore/libs/daemon
    cloud/filestore/libs/diagnostics
    cloud/filestore/libs/endpoint
    cloud/filestore/libs/endpoint_vhost
    cloud/filestore/libs/server

    cloud/storage/core/libs/common
    cloud/storage/core/libs/daemon
    cloud/storage/core/libs/diagnostics
    cloud/storage/core/libs/keyring
    cloud/storage/core/libs/version

    library/cpp/lwtrace/mon

    library/cpp/actors/util
    library/cpp/getopt
    library/cpp/logger
    library/cpp/protobuf/util
    library/cpp/sighandler
)
