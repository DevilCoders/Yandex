PY3_PROGRAM(py3_program)

OWNER(
    g:blemder
)

PEERDIR(
    kernel/blender/factor_storage/pylib
)

SRCDIR(kernel/blender/factor_storage/test/integration_test/python_program)

PY_SRCS(__main__.py)

END()
