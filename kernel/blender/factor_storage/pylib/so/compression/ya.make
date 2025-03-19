PY2MODULE(compression)

OWNER(
    dima-zakharov
)

PYTHON2_ADDINCL()

PEERDIR(
    kernel/blender/factor_storage
)

SRCDIR(
    kernel/blender/factor_storage/pylib
)

SRCS(
    compression.pyx
)

EXPORTS_SCRIPT(compression.exports)

END()
