LIBRARY()

OWNER(g:util)

SRCS(
    kmp.cpp
    kmp.h
)

END()

RECURSE_FOR_TESTS(
    ut
)
