LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/config
    kernel/snippets/qtree
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/span
)

SRCS(
    wordstat.cpp
    wordstat_data.cpp
)

END()
