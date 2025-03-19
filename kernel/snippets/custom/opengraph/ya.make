LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/config
    kernel/snippets/cut
    kernel/snippets/delink
    kernel/snippets/sent_match
    kernel/snippets/simple_cmp
    kernel/snippets/titles/make_title
    library/cpp/string_utils/url
)

SRCS(
    og.cpp
)

END()
