PY23_LIBRARY()

OWNER(orivej)

PEERDIR(
    contrib/libs/llvm12/lib/Demangle
)

PY_SRCS(
    __init__.py
    _demangle.pyx
)

END()

RECURSE_FOR_TESTS(
    test
)
