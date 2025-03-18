OWNER(
    g:base
    g:wizard
    onpopov
)

PROGRAM()

SRCS(
    printrichnodes.cpp
)

PEERDIR(
    kernel/lemmer
    kernel/qtree/request
    kernel/qtree/richrequest
    kernel/reqerror
    kernel/search_daemon_iface
    library/cpp/charset
    library/cpp/getopt
    ysite/yandex/pure
)

END()
