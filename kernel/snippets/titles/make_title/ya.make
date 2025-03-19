LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/lemmer/core
    kernel/lemmer/untranslit
    kernel/snippets/config
    kernel/snippets/idl
    kernel/snippets/plm
    kernel/snippets/qtree
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/smartcut
    kernel/snippets/wordstat
    library/cpp/lcs
    library/cpp/string_utils/url
)

SRCS(
    make_title.cpp
    util_title.cpp
)

END()
