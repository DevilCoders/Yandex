UNITTEST_FOR(kernel/random_log_factors)

OWNER(
    alejes
    g:neural-search
)

SRCS(
    random_log_meta_factors_ut.cpp
)

PEERDIR(
    kernel/random_log_factors
    kernel/random_log_factors/proto
)

END()

NEED_CHECK()
