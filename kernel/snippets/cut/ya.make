LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/algo
    kernel/snippets/config
    kernel/snippets/iface
    kernel/snippets/qtree
    kernel/snippets/sent_match
    kernel/snippets/smartcut
    kernel/snippets/strhl
    kernel/snippets/uni_span_iter
    kernel/snippets/weight
)

SRCS(
    cut.cpp
)

END()
