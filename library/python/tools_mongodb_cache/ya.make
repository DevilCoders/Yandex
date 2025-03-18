PY23_LIBRARY()

OWNER(g:tools-python)

PEERDIR(
    contrib/python/pymongo
    contrib/python/six
)

PY_SRCS(
    TOP_LEVEL
    tools_mongodb_cache/__init__.py
    tools_mongodb_cache/cache.py
    tools_mongodb_cache/mongodb.py
)

END()
