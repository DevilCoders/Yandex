LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/archive/view
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/titles/make_title
)

SRCS(
    title_trigram.cpp
)

END()
