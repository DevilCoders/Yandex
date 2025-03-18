PY23_LIBRARY()

OWNER(
    g:crypta
    g:crypta-lib
    ptyavin
    eugene311
)

PEERDIR(
    library/cpp/bloom_filter
)

PY_SRCS(
    bloom.pyx
    __init__.py
)

END()

RECURSE(test)
