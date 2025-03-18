OWNER(g:snippets)

LIBRARY()

SRCS(
    snippet_dump_xml_writer.cpp
)

PEERDIR(
    contrib/libs/libxml
    kernel/snippets/util
    tools/snipmake/common
)

END()
