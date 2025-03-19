LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/archive/markup
    kernel/snippets/archive/unpacker
    kernel/snippets/archive/view
    kernel/snippets/config
    kernel/snippets/iface/archive
    kernel/snippets/qtree
    kernel/snippets/sent_match
    kernel/snippets/smartcut
    kernel/snippets/titles/make_title
    library/cpp/charset
    library/cpp/langmask
)

SRCS(
    video.cpp
)

END()
