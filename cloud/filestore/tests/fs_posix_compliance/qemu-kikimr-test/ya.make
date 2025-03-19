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

SET(QEMU_VIRTIO fs)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tools/testing/fs_posix_compliance/fs_posix_compliance.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/service-kikimr.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/vhost.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/vhost-endpoint.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/storage/core/tests/recipes/qemu.inc)

END()
