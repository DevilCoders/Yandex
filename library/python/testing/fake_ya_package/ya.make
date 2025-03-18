OWNER(g:yatest)

PY23_LIBRARY()

PEERDIR(
    devtools/ya/exts
)

PY_SRCS(
    __init__.py
)

END()

RECURSE_FOR_TESTS(test)

