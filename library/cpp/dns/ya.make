LIBRARY()

OWNER(and42)

SRCS(
    cache.cpp
    thread.cpp
    magic.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
