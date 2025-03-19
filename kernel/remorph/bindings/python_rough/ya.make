INCLUDE(${ARCADIA_ROOT}/kernel/remorph/version.cmake)

PY2MODULE(yandex_remorph_python_rough ${REMORPH_EXTVER_MAJOR} ${REMORPH_EXTVER_MINOR} PREFIX lib)

OWNER(g:remorph)

ENABLE(MAKE_ONLY_SHARED_LIB)

SRCDIR(kernel/remorph/bindings/swig)

SRCS(
    remorph_rough.swg
)

PEERDIR(
    kernel/remorph/api
)

END()
