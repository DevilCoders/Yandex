OWNER(
    g:base
    g:wizard
    onpopov
)

PROGRAM()

SRCS(
    printreqs.cpp
)

PEERDIR(
    library/cpp/getopt
    kernel/reqerror
    kernel/qtree/request
)

END()
