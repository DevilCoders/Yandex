PY3_LIBRARY()

OWNER(g:billing-bcl)

PEERDIR(
    contrib/python/requests
    contrib/python/pytest-responsemock
    library/python/tvm2
)

PY_SRCS(
    TOP_LEVEL
    bclclient/__init__.py
    bclclient/auth.py
    bclclient/client.py
    bclclient/exceptions.py
    bclclient/http.py
    bclclient/settings.py

    bclclient/resources/__init__.py
    bclclient/resources/base.py
    bclclient/resources/payments.py
    bclclient/resources/proxy.py
    bclclient/resources/reference.py
    bclclient/resources/statements.py
    bclclient/resources/utils.py
)

END()

RECURSE_FOR_TESTS(tests)
