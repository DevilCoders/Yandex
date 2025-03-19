UNITTEST_FOR(kernel/nn/neural_ranker/concat_features)

OWNER(g:factordev)

PEERDIR(
    library/cpp/testing/unittest
    kernel/nn/neural_ranker/concat_features
)

SIZE(SMALL)

SRCS(
    features_concater_ut.cpp
)

END()
