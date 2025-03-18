PY3TEST()

OWNER(
    dskut
    g:mail
)

TEST_SRCS(
    base_test.py
    test_pyremock.py
    test_assert_expectations.py
)

PEERDIR(
    library/python/testing/pyremock/lib
)

END()
