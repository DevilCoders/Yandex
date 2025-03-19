OWNER(g:cloud-marketplace)

PY3_PROGRAM(functest)

PY_SRCS(
    __main__.py
    config.py
)

PEERDIR(
    library/python/testing/recipe
    library/python/testing/yatest_common
    contrib/python/requests
)

END()
