PY23_LIBRARY()

OWNER(g:billing-bcl)

PEERDIR(
    contrib/python/requests
    contrib/python/click
)

PY_SRCS(
    TOP_LEVEL
    refsclient/__init__.py
    refsclient/cli.py
    refsclient/client.py
    refsclient/compat.py
    refsclient/exceptions.py
    refsclient/http.py
    refsclient/refs/__init__.py
    refsclient/refs/_base.py
    refsclient/refs/cbrf.py
    refsclient/refs/currency.py
    refsclient/refs/fias.py
    refsclient/refs/swift.py
    refsclient/refs/utils.py
    refsclient/settings.py
    refsclient/utils.py
)

END()

RECURSE_FOR_TESTS(tests)
