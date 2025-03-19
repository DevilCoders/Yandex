LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/config
    kernel/snippets/replace
    kernel/snippets/sent_match
    kernel/snippets/simple_cmp
)

SRCS(
    delink.cpp
)

END()
