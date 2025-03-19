PY3_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    errors.py
    test_cases.py
)

PEERDIR(
    cloud/blockstore/pylibs/ycp

    yt/python/client
)

END()

