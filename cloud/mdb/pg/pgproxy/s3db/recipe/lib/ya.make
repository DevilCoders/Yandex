OWNER(
    g:mdb
    g:s3
)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    library/python/testing/recipe
    library/python/testing/yatest_common
)

PY_SRCS(
    __init__.py
    host.py
    shard.py
    cluster.py
    config.py
)

END()
