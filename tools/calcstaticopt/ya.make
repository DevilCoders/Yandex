PROGRAM()

NO_JOIN_SRC()

SRCS(
    main.cpp
    scanner.rl6
    parser.y
)

PEERDIR(
    kernel/relevfml
    kernel/web_factors_info
    library/cpp/getopt/small
)

INDUCED_DEPS(h+cpp
    ${ARCADIA_ROOT}/library/cpp/sse/sse.h
    ${ARCADIA_ROOT}/kernel/relevfml/relev_fml.h
)

END()
