PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(0.44)

PEERDIR(
    library/python/yenv
    contrib/python/click
    contrib/python/kazoo
    contrib/python/pymongo
    yt/python/yt/wrapper
)

PY_SRCS(
    TOP_LEVEL
    ylock/__init__.py
    ylock/base.py
    ylock/decorators.py
    ylock/main.py
    ylock/backends/__init__.py
    ylock/backends/mongodb.py
    ylock/backends/thread.py
    ylock/backends/yt.py
    ylock/backends/zookeeper.py
)

END()

RECURSE_FOR_TESTS(tests)
