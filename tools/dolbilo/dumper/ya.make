PROGRAM(d-dumper)

OWNER(
    g:base
    mvel
)

PEERDIR(
    library/cpp/dolbilo
    library/cpp/getopt
    library/cpp/streams/factory
    library/cpp/http/push_parser
)

SRCS(
    main.cpp
)

GENERATE_ENUM_SERIALIZATION(responses_mode.h)

END()
