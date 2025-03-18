PY3_LIBRARY()

OWNER(
    g:s3
    paxakor
)

PEERDIR(
    contrib/python/aioboto3
    contrib/python/aiobotocore
    contrib/python/pytz
)

PY_SRCS(
    __init__.py
)

END()
