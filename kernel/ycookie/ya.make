LIBRARY()

OWNER(
    g:atom
)

SRCS(
    ycookie.cpp
)

PEERDIR(
    library/cpp/string_utils/quote
)

END()

RECURSE(
    ut
)
