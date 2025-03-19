UNITTEST()

OWNER(g:fintech-bnpl)

PEERDIR(
    kernel/common_server/util/json_scanner
    library/cpp/json
)

SRCS(
    simple_ut.cpp
)

SIZE(SMALL)

END()
