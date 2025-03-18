OWNER(
    g:base
    g:wizard
)

PROGRAM()

SRCS(
    nodeiter_test.cpp
)

PEERDIR(
    kernel/lemmer
    kernel/qtree/request
    kernel/qtree/richrequest
    kernel/reqerror
    kernel/search_daemon_iface
    library/cpp/charset
    library/cpp/getopt
)

END()
