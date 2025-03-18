LIBRARY()

OWNER(g:kikimr)

SRCS(
    sliding_window.cpp
    sliding_window.h
)

END()

RECURSE_FOR_TESTS(
    ut
)
