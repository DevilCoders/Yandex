OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
)

END()

RECURSE(
    common
    solomon
    juggler
    yasm
)
