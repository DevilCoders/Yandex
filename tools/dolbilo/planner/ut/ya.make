UNITTEST()

OWNER(
    g:base
    mvel
)

PEERDIR(
    library/cpp/testing/unittest
    library/cpp/dolbilo/planner
)

SIZE(SMALL)

SRCS(
    plain_loader_ut.cpp
)

END()
