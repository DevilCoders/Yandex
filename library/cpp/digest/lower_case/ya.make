LIBRARY()

OWNER(
    pg
    g:util
)

SRCS(
    lciter.cpp
    lchash.cpp
    hash_ops.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
