PY3TEST()

OWNER(g:cloud-nbs)

SIZE(LARGE)
TIMEOUT(60)

REQUIREMENTS(
    container:2273379275
)

TAG(ya:fat ya:force_sandbox)

TEST_SRCS(
    test.py
)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/service-kikimr.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/nfs-ganesha.inc)

END()
