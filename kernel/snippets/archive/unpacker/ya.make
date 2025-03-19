LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/archive/markup
    kernel/snippets/archive/view
    kernel/snippets/archive/zone_checker
    kernel/snippets/config
    kernel/snippets/iface
    kernel/snippets/iface/archive
    kernel/tarc/iface
    library/cpp/charset
)

SRCS(
    unpacker.cpp
)

END()
