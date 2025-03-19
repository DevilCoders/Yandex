LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/config
    kernel/snippets/qtree
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/wordstat
)

SRCS(
    read_helper.cpp
)

END()
