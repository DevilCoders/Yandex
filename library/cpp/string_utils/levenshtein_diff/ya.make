LIBRARY()

OWNER(g:mt)

SRCS(
    levenshtein_diff.cpp
)

PEERDIR(
    util/draft
)

END()

RECURSE_FOR_TESTS(
    ut
)
