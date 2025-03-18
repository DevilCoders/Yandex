LIBRARY()

PEERDIR(
    contrib/libs/openssl
    contrib/libs/re2
    tools/snipmake/argv
    tools/snipmake/common
    tools/snipmake/snippet_xml_parser/cpp_reader
    tools/snipmake/steam/protos
    tools/snipmake/steam/serp_parser
    tools/snipmake/steam/snippet_json_iterator
)

OWNER(g:snippets)

SRCS(
    storage.cpp
)

END()
