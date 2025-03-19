LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/string_utils/quote
    search/web/util/config_parser
)

SRCS(
    attribute.cpp
    rearrange.cpp
)

END()
