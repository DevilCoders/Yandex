OWNER(g:mdb g:s3)

PY2_LIBRARY()

PEERDIR(
    contrib/python/sentry-sdk
)

PY_SRCS(
    NAMESPACE util
    chunk.py
    const.py
    database.py
    exceptions.py
    helpers.py
    pgmeta.py
    s3db.py
    s3meta.py
)

END()
