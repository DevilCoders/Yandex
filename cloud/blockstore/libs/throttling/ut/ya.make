UNITTEST_FOR(cloud/blockstore/libs/throttling)

OWNER(g:cloud-nbs)

SRCS(
    throttler_ut.cpp
    throttler_formula_ut.cpp
    throttler_logger_ut.cpp
)

PEERDIR(
    library/cpp/threading/future/subscription
)

END()
