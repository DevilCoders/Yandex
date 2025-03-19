LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/info_request
    kernel/snippets/algo
    kernel/snippets/archive/markup
    kernel/snippets/archive/unpacker
    kernel/snippets/archive/view
    kernel/snippets/config
    kernel/snippets/factors
    kernel/snippets/formulae
    kernel/snippets/iface
    kernel/snippets/iface/archive
    kernel/snippets/markers
    kernel/snippets/qtree
    kernel/snippets/replace
    kernel/snippets/schemaorg
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/strhl
    kernel/snippets/titles/make_title
    kernel/snippets/util
    kernel/snippets/wordstat
    kernel/tarc/iface
    library/cpp/charset
    library/cpp/html/pcdata
    library/cpp/scheme
    library/cpp/streams/bzip2
    library/cpp/string_utils/base64
    library/cpp/svnversion
)

SRCS(
    cookie.cpp
    dump.cpp
    finaldump.cpp
    finaldumpbin.cpp
    losswords.cpp
    pooldump.cpp
    semantic.cpp
    table.cpp
    texthits.cpp
    top_candidates.cpp
)

END()
