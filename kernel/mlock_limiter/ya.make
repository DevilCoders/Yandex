LIBRARY()

OWNER(
    g:base
    g:saas2
)

SRCS(
    mlock_limiter.cpp
)

END()

RECURSE_FOR_TESTS(ut)
