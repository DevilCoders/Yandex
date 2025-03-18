PY23_LIBRARY()

OWNER(yanush77)

TEST_SRCS(
    test_murmurhash.py
)

PEERDIR(
    library/python/murmurhash
)

END()

RECURSE_FOR_TESTS(
    py2
    py3
)
