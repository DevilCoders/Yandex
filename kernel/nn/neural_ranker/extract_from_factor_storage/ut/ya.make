UNITTEST_FOR(kernel/nn/neural_ranker/extract_from_factor_storage)

OWNER(g:factordev)

PEERDIR(
    kernel/nn/neural_ranker/extract_from_factor_storage
    library/cpp/testing/unittest
)

SIZE(SMALL)

SRCS(
    factor_storage_extractor_ut.cpp
)

END()
