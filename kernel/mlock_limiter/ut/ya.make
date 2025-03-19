GTEST()

SIZE(small)

OWNER(
    sskvor
    g:base
)

SRCS(
    mlock_limiter_ut.cpp
)

PEERDIR(
    kernel/mlock_limiter
)


END()
