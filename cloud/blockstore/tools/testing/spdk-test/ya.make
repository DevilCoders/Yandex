PROGRAM()

OWNER(g:cloud-nbs)

GENERATE_ENUM_SERIALIZATION(options.h)

SRCS(
    app.cpp
    device.cpp
    initiator.cpp
    main.cpp
    options.cpp
    runnable.cpp
    target.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/spdk
    library/cpp/getopt
    library/cpp/deprecated/atomic
)

END()
