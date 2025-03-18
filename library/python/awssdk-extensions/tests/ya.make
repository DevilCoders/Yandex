PY2TEST()

OWNER(g:s3)

PEERDIR(
    contrib/python/boto3
    contrib/python/botocore
)

TEST_SRCS(
    test_botocore_api.py
)

SIZE(SMALL)

END()

