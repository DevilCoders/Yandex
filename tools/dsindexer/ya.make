#ENABLE(USEMPROF)

PROGRAM()

OWNER(leo)

IF (USEMPROF)
    CFLAGS(-DUSE_MPROF)
ENDIF()

SRCS(
    dsindexer.cpp
    main.cpp
)

PEERDIR(
    library/cpp/getopt
    library/cpp/svnversion
    ysite/datasrc/dsconf
    ysite/indexer
)

END()
