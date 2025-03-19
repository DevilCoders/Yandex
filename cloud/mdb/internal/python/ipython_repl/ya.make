PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/ipython
)

PY_SRCS(
    __init__.py
    repl.py
)

END()
