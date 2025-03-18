LIBRARY()

OWNER(g:yt)

PEERDIR(
    contrib/libs/farmhash
)

CHECK_DEPENDENT_DIRS(
    ALLOW_ONLY ALL
    build
    contrib
    library
    util
)

END()

RECURSE_FOR_TESTS(
    unittests
)

