PROGRAM(metasnip-parse)

OWNER(divankov)

PEERDIR(
    kernel/snippets/strhl
    library/cpp/cgiparam
    library/cpp/charset
    library/cpp/getopt
    library/cpp/http/io
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    search/idl
    search/session/compression
    tools/snipmake/snipdat
    tools/snipmake/snippet_xml_parser/cpp_writer
)

SRCS(
    main.cpp
)

END()
