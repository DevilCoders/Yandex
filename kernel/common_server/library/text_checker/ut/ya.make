UNITTEST()

OWNER(g:fintech-bnpl)

PEERDIR(
    kernel/common_server/library/text_checker
    library/cpp/testing/unittest
)

SRCS(
    simple_ut.cpp
)

SIZE(SMALL)

END()
