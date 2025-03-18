PY23_LIBRARY()

OWNER(dmtrmonakhov)

PEERDIR(
    library/python/testing/fake_ya_package
)

TEST_SRCS(
    test_package.py
)

END()

RECURSE_FOR_TESTS(
    py2
    py3
)
