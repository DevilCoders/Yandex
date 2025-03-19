PY23_LIBRARY()

OWNER(g:cloud-billing)

PY_SRCS(
    __init__.py
    base.py
)

PEERDIR(
    contrib/python/schematics
)
END()
