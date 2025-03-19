LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/config
    kernel/snippets/qtree
    kernel/snippets/replace
    kernel/snippets/sent_match
    kernel/snippets/titles/make_title
    library/cpp/string_utils/url
)

SRCS(
    remove_emoji.cpp
)

END()
