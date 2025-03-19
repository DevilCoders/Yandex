OWNER(qqq)

PY23_LIBRARY()

SRCDIR(kernel/remorph/bindings/swig)

PY_SRCS(
    TOP_LEVEL
    remorph_rough.swg=yandex_remorph_python_rough
)

PEERDIR(
    kernel/remorph/api
)

END()
