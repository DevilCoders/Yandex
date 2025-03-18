PROGRAM()

SRCS(
    printxref.cpp
)

PEERDIR(
    kernel/keyinv/hitlist
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
    kernel/search_types
    kernel/tarc/disk
    kernel/xref
    library/cpp/charset
    library/cpp/containers/spars_ar
    library/cpp/getopt
    library/cpp/string_utils/old_url_normalize
    ysite/yandex/common
    ysite/yandex/srchmngr
)

END()
