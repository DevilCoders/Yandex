LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/archive/unpacker
    kernel/snippets/archive/view
    kernel/snippets/config
    kernel/snippets/custom/forums_handler
    kernel/snippets/iface/archive
    kernel/snippets/schemaorg
    kernel/snippets/sent_info
    kernel/tarc/iface
)

SRCS(
    preview_viewer.cpp
)

END()
