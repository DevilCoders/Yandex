PY3TEST()

OWNER(g:cloud-nbs)

SIZE(MEDIUM)
TIMEOUT(600)

FORK_SUBTESTS()

PEERDIR(
    cloud/filestore/tests/python/lib
)

TEST_SRCS(
    test.py
)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tools/testing/fs_posix_compliance/fs_posix_compliance.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/service-local.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/mount.inc)

END()
