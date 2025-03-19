OWNER(g:remorph)

INCLUDE(${ARCADIA_ROOT}/kernel/remorph/version.cmake)

DLL_FOR(kernel/remorph/api yandex-remorph ${REMORPH_EXTVER_MAJOR} ${REMORPH_EXTVER_MINOR}
    EXPORTS remorph.exports
)
