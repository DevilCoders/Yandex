PROGRAM()

OWNER(
    divankov
    g:snippets
)

SRCS(
    urlcut_test.cpp
)

PEERDIR(
    kernel/qtree/richrequest
    kernel/snippets/urlcut
    library/cpp/cgiparam
)

END()

RECURSE(
    tests
)
