UNITTEST()

OWNER(g:fintech-bnpl)

PEERDIR(
    library/cpp/json

    kernel/common_server/rt_background
    kernel/common_server/api
    fintech/risk/backend/src/proto
    kernel/common_server/rt_background/processes/dumper
)

SRCS(
    meta_parser_ut.cpp
)

SIZE(SMALL)

END()
