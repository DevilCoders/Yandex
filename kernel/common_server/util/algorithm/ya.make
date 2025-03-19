LIBRARY()

OWNER(g:cs_dev)

SRCS(
    container.h
    iterator.h
)

END()

RECURSE_FOR_TESTS(
    ut
)
