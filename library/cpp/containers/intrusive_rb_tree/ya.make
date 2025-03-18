LIBRARY()

OWNER(
    pg
    g:util
)

SRCS(
    rb_tree.cpp
)

END()

RECURSE(
    fuzz
    ut
)
