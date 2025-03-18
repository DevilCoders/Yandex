PY23_LIBRARY()

VERSION(0.10)

OWNER(
    dshmatkov
)

PY_SRCS(
    TOP_LEVEL
    yenv.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
