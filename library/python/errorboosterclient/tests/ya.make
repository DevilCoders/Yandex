OWNER(g:billing-bcl)

PY23_TEST()

# kikimr import failure workaround
NO_CHECK_IMPORTS(*)

PEERDIR(
    contrib/python/sentry-sdk
    contrib/python/mock
    library/python/errorboosterclient
)

TEST_SRCS(
    test_logbroker.py
    test_sentry.py
    test_uagent.py
)

END()
