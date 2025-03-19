PY2TEST()

OWNER(
    g:blender
)

PEERDIR(
    kernel/blender/factor_storage/pylib
    kernel/blender/factor_storage/test/common
)

SRCDIR(kernel/blender/factor_storage/pylib/test)

TEST_SRCS(test.py)

END()
