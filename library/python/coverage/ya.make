PY23_LIBRARY()

OWNER(g:yatool)

PY_CONSTRUCTOR(library.python.coverage)

PY_SRCS(
    __init__.py
)

PEERDIR(
    contrib/python/coverage
)

END()
