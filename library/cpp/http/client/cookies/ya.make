LIBRARY()

OWNER(
    stanly
)

SRCS(
    cookiestore.cpp
    parser.rl6
)

PEERDIR(
    library/cpp/deprecated/split
    library/cpp/uri
)

END()

RECURSE_FOR_TESTS(ut)
