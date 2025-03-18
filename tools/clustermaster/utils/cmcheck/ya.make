PROGRAM()

OWNER(
    g:clustermaster
    g:kwyt
)

PEERDIR(
    ADDINCL tools/clustermaster/common
    tools/clustermaster/common
    tools/clustermaster/master/lib
    library/cpp/getopt/small
    library/cpp/uri
    library/cpp/mime/types
)

SRCS(
    cmcheck.cpp
)

END()
