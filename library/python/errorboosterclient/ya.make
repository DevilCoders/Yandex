PY23_LIBRARY()

OWNER(g:billing-bcl idlesign)

# deliberately no PEERDIR to the following
# (maybe the client would be standalone in future):
#
#   * contrib/python/sentry-sdk
#   * kikimr/public/sdk/python/persqueue

IF(PYTHON2)
    PEERDIR(
        contrib/python/typing
    )
ENDIF()

PY_SRCS(
    TOP_LEVEL
    errorboosterclient/__init__.py
    errorboosterclient/sentry.py
    errorboosterclient/logbroker.py
    errorboosterclient/uagent.py
)

END()

RECURSE_FOR_TESTS(tests)
