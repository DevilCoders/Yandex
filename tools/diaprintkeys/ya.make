PROGRAM()

ALLOCATOR(GOOGLE)

SRCS(
    diaprintkeys.cpp
)

PEERDIR(
    kernel/keyinv/hitlist
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
    kernel/search_types
    library/cpp/charset
    library/cpp/getopt
    library/cpp/svnversion
    ysite/yandex/common
)

END()
