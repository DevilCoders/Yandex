PY3TEST()

SIZE(MEDIUM)

OWNER(g:cloud-nbs)

FORK_SUBTESTS()

REQUIREMENTS(
    cpu:4 ram:10
)

TEST_SRCS(
    test.py
)

DEPENDS(
    cloud/filestore/tools/testing/loadtest/bin
)

DATA(
    arcadia/cloud/filestore/tests/loadtest/service-kikimr-test
)

PEERDIR(
    cloud/filestore/tests/python/lib
)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/service-kikimr.inc)

END()
