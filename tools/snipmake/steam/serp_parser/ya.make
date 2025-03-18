LIBRARY()

OWNER(g:snippets)

SRCS(
    xmlserps.xsyn
    serp_xml_reader.cpp
)

PEERDIR(
    library/cpp/logger
    library/cpp/xml/parslib
    tools/snipmake/common
)

END()
