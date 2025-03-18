OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
    consts.py
    diff.py
    text.py
    arcanum.py
)

PEERDIR(
    contrib/python/click
)

END()
