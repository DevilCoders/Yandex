UNITTEST_FOR(kernel/prs_log/serializer)

OWNER(
    crossby
    g:factordev
)

PEERDIR(
    library/cpp/testing/unittest
    kernel/web_factors_info
    kernel/web_meta_factors_info
)

SIZE(SMALL)

SRCS(
    test_serializer_ut.cpp
)

END()
