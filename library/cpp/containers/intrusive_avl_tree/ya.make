LIBRARY()

OWNER(
    pg
    g:util
)

SRCS(
    avltree.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
