PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/dbaas_python_common
    contrib/python/requests
)

PY_SRCS(
    __init__.py
    client.py
    errors.py
)

END()

RECURSE_FOR_TESTS(
    test
)
