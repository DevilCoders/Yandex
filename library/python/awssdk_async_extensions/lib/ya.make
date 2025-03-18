PY3_LIBRARY()

OWNER(
    g:s3
    paxakor
)

PEERDIR(
    library/python/awssdk_async_extensions/lib/core
    library/python/deprecated/ticket_parser2
    library/python/tvm2
)

PY_SRCS(
    __init__.py
)

END()
