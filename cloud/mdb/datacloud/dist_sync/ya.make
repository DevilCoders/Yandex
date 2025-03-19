OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    contrib/python/raven
    contrib/python/pyaml
)

ALL_PY_SRCS()

END()

RECURSE(
    bin
)

RECURSE_FOR_TESTS(
    tests
)
