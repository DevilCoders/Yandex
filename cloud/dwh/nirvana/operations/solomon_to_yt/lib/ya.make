OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    library/python/nirvana
    yt/python/client
    contrib/python/arrow
    contrib/python/dateutil
    contrib/python/tenacity

    cloud/dwh/clients/solomon
    cloud/dwh/utils
)

PY_SRCS(
    __init__.py
    operation.py
    exceptions.py
    types.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
