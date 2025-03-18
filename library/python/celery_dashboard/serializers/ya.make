PY3_LIBRARY()

OWNER(g:sport)

PY_SRCS(
    __init__.py

    base_serializer.py
    json_serializer.py
    msgpack_serializer.py
    pickle_serializer.py
)

PEERDIR(
    contrib/python/msgpack
)

NO_CHECK_IMPORTS()

END()
