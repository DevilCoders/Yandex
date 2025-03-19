PY23_LIBRARY()

OWNER(
    g:blender
)

PEERDIR(
    kernel/blender/factor_storage
    kernel/blender/factor_storage/protos
)

PY_SRCS(
    __init__.py
    compression.pyx
    serialization.py
)

END()

RECURSE(
    so
)

RECURSE_FOR_TESTS(
    test
)
