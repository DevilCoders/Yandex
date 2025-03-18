PROGRAM()

SRCS(
    printlinks.cpp
)

PEERDIR(
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
    kernel/search_types
    library/cpp/getopt
    library/cpp/string_utils/old_url_normalize
    ysite/yandex/common
)

END()
