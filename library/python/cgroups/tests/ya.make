PY23_LIBRARY()

OWNER(dmitko)

TEST_SRCS(test_cgroups.py)

PEERDIR(
    library/python/cgroups
)

END()

RECURSE_FOR_TESTS(
    py2
    py3
)
