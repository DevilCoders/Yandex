PY23_LIBRARY()

OWNER(borman)

PY_SRCS(run.pyx)

END()

RECURSE(
    example
)

RECURSE_FOR_TESTS(
    test/py2
    test/py3
)
