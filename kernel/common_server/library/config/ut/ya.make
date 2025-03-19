UNITTEST()

OWNER(g:cs_dev)

SRCS(
    context_value_ut.cpp
    context_value_parser_json_ut.cpp
)

TAG(
    ya:sandbox_coverage
)

PEERDIR(
    kernel/common_server/library/config
)

END()
