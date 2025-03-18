OWNER(
    onpopov
    yazevnul
)

PROGRAM()

IF (USEMPROF)
    CFLAGS(-DUSE_MPROF)
ENDIF()

SRCS(
    main.cpp
)

PEERDIR(
    tools/trietest/lib
    library/cpp/getopt
)

END()
