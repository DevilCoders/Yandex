LIBRARY()

OWNER(g:facts)

SRCS(
    constants.h 
    normalize_text.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
