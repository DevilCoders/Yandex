PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    contrib/python/boto3
)

PY_SRCS(
    s3.py
    __init__.py
)

END()
