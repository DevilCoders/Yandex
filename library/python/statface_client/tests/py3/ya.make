PY3TEST()

OWNER(
    g:statlibs
    g:statinfra
)

NO_BUILD_IF(OS_WINDOWS)

INCLUDE(${ARCADIA_ROOT}/library/python/statface_client/tests/common/ya.make.inc)

END()

