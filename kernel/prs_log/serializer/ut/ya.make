UNITTEST_FOR(kernel/prs_log/serializer)

OWNER(
    ayles
    g:middle
)

PEERDIR(
    library/cpp/testing/unittest
    kernel/prs_log/serializer
)

SIZE(SMALL)

SRCS(
    test_proto_serialization_ut.cpp
)

END()
