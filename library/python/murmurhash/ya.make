PY23_LIBRARY()

OWNER(yanush77)

PY_SRCS(
    NAMESPACE murmurhash
    __init__.py
)

SRCS(
    _murmurhash.cpp
)

PY_REGISTER(_murmurhash)

END()

RECURSE_FOR_TESTS(ut)
