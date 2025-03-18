PY23_LIBRARY()

OWNER(g:solomon)

PY_SRCS(
    TOP_LEVEL
    solomon/__init__.py
    solomon/solomon.py
)

PEERDIR(
    contrib/python/furl
    contrib/python/requests
    contrib/python/six
)

IF (PYTHON2)
    PEERDIR(
        contrib/python/enum34
    )
ENDIF()

END()

RECURSE_FOR_TESTS(
    tests
)
