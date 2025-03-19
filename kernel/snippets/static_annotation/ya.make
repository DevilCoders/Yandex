LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/archive/markup
    kernel/snippets/archive/unpacker
    kernel/snippets/archive/view
    kernel/snippets/config
    kernel/snippets/custom/trash_classifier
    kernel/snippets/cut
    kernel/snippets/iface/archive
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/title_trigram
    kernel/snippets/titles/make_title
    kernel/tarc/markup_zones
)

SRCS(
    static_annotation.cpp
)

END()
