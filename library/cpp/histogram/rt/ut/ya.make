UNITTEST()

OWNER(ivanmorozov)

PEERDIR(
    library/cpp/histogram/rt
)

SRCDIR(library/cpp/histogram/rt)

SRCS(
    histogram_ut.cpp
)

FORK_SUBTESTS()

SPLIT_FACTOR(50)

TIMEOUT(600)

SIZE(MEDIUM)

END()
