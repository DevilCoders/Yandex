PY23_LIBRARY()

OWNER(g:s3)

PEERDIR(
    contrib/python/boto3
    contrib/python/botocore
    contrib/python/pytz
    library/python/deprecated/ticket_parser2
)

SRCDIR(
    library/python/awssdk-extensions/src
)

PY_SRCS(
    NAMESPACE boto3_tvm2
    __init__.py
    session.py
)

END()

RECURSE(
    example
)

RECURSE_FOR_TESTS(
    tests
)
