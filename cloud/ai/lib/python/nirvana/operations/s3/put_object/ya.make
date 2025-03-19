PY3_PROGRAM()

PY_SRCS(
    MAIN put_object.py
)

PEERDIR(
    cloud/ai/lib/python/datasource/s3
    library/python/nirvana
)

END()
