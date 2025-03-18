PY23_LIBRARY()

OWNER(borman)

PEERDIR(
    contrib/python/Flask
    library/python/resource
)

PY_SRCS(
    static.py
    __init__.py
)

END()
