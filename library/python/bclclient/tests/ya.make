OWNER(g:billing-bcl)

PY3TEST()

PEERDIR(
    library/python/bclclient
)

TEST_SRCS(
    conftest.py
    test_payments.py
    test_proxy.py
    test_reference.py
    test_statements.py
    test_utils.py
)

END()
