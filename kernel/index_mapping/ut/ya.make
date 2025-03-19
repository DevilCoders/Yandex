UNITTEST()

OWNER(
    ivanmorozov
    g:saas
)

SRCS (
    index_mapping_ut.cpp
)

PEERDIR (
    kernel/index_mapping
)

SIZE(SMALL)

FORK_SUBTESTS()
SPLIT_FACTOR(7)

END()
