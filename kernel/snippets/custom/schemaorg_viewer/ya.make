LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/lemmer/alpha
    kernel/snippets/archive/unpacker
    kernel/snippets/archive/view
    kernel/snippets/config
    kernel/snippets/iface/archive
    kernel/snippets/schemaorg
    kernel/snippets/schemaorg/question
    library/cpp/string_utils/url
)

SRCS(
    schemaorg_viewer.cpp
)

END()
