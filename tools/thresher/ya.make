PROGRAM()

IF (USEMPROF)
    CFLAGS(-DUSE_MPROF)
ENDIF()

PEERDIR(
    library/cpp/charset
    library/cpp/getopt
)

SRCS(
    thresher.cpp
)

END()
