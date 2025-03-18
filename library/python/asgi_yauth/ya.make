PY3_LIBRARY()

OWNER(
    smosker
    g:tools-python
)

VERSION(1.0)

PEERDIR(
    library/python/tvm2

    contrib/python/pydantic
)

PY_SRCS(
    TOP_LEVEL
    asgi_yauth/__init__.py
    asgi_yauth/backends/base.py
    asgi_yauth/backends/tvm2.py
    asgi_yauth/exceptions.py
    asgi_yauth/middleware.py
    asgi_yauth/settings.py
    asgi_yauth/types.py
    asgi_yauth/user.py
    asgi_yauth/utils/__init__.py
    asgi_yauth/utils/headers.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
