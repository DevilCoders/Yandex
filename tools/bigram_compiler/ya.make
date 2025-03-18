PROGRAM()

IF (USEMPROF)
    CFLAGS(-DUSE_MPROF)
ENDIF()

SRCS(
    main.cpp
)

PEERDIR(
    dict/query_bigrams
    library/cpp/containers/comptrie
    library/cpp/getopt/small
)

END()
