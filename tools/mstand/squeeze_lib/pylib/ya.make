PY3_LIBRARY()

OWNER(g:mstand)

PEERDIR(
    tools/mstand/squeeze_lib/lib
)

PY_SRCS(
    __init__.py
    bindings.pyx
)

END()

RECURSE_FOR_TESTS(
    ut
)
