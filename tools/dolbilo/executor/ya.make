PROGRAM(d-executor)

OWNER(
    g:base
    mvel
    darkk
)

PEERDIR(
    library/cpp/coroutine/engine
    library/cpp/dolbilo
    library/cpp/getopt
    library/cpp/http/io
    library/cpp/http/misc
    library/cpp/streams/factory
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
    tools/dolbilo/libs/rps_schedule
)

GENERATE_ENUM_SERIALIZATION(echomode.h)

SRCS(
    main.cpp
    dns.cpp
    slb.cpp
)

END()
