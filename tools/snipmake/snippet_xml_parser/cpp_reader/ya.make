LIBRARY()

OWNER(divankov)

SRCS(
    xmlsnippets.xsyn
    snippet_xml_reader.cpp
)

PEERDIR(
    kernel/qtree/richrequest
    kernel/snippets/util
    library/cpp/logger
    library/cpp/xml/parslib
    tools/snipmake/common
    util/draft
)

END()
