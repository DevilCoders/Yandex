PROGRAM()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/idl
    kernel/snippets/schemaorg
    kernel/tarc/iface
    kernel/tarc/markup_zones
    library/cpp/getopt
    library/cpp/langmask
    library/cpp/langs
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
)

SRCS(
    filterctx.cpp
)

END()
