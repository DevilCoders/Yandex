PROGRAM()

OWNER(ironpeter)

ALLOCATOR(LF)

SRCS(
    indexconvert.cpp
    indexlemmas.cpp
)

PEERDIR(
    kernel/keyinv/hitlist
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
    kernel/lemmer
    kernel/search_types
    library/cpp/charset
    library/cpp/containers/mh_heap
    library/cpp/getopt
    library/cpp/token
    library/cpp/tokenizer
    ysite/yandex/common
    ysite/yandex/posfilter
)

END()
