LIBRARY()

OWNER(pg)

SRCS(
    colors.cpp
    output.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
