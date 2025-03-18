OWNER(g:billing-bcl)

PY2TEST()

PEERDIR(
    library/python/refsclient
)

TAG(
    ya:external
)

REQUIREMENTS(
    network:full
)

TEST_SRCS(
    conftest.py
    test_basics.py
    test_cbrf.py
    test_currency.py
    test_fias.py
    test_swift.py
)

END()
