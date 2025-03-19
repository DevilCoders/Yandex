LIBRARY()

OWNER(
    filmih
    g:factordev
    g:neural-search
)

PEERDIR(
    kernel/factor_storage
    kernel/factors_selector/proto
)

SRCS(
    factors.cpp
)

END()

RECURSE(
    proto
)

RECURSE_FOR_TESTS(
    ut
)
