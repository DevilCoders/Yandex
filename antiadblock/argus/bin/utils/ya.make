PY3_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    exceptions.py
    s3_interactions.py
    utils.py
)

PEERDIR(
    contrib/python/boto3
    contrib/python/botocore
    contrib/python/retry

    antiadblock/libs/adb_selenium_lib
    antiadblock/libs/utils
)

END()
