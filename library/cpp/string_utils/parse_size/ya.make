LIBRARY()

OWNER(g:images-robot)

SRCS(
    parse_size.cpp
    parse_size.h
)

END()

RECURSE_FOR_TESTS(
    ut
)
