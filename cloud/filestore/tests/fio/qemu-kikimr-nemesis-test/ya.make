PY3TEST()

OWNER(g:cloud-nbs)

SIZE(MEDIUM)
TIMEOUT(600)

FORK_SUBTESTS()

DEPENDS(
    cloud/storage/core/tools/testing/fio/bin
)

PEERDIR(
    cloud/filestore/tests/python/lib
    cloud/storage/core/tools/testing/fio/lib
)

TEST_SRCS(
    test.py
)

SET(QEMU_VIRTIO fs)
SET(VHOST_RESTART_INTERVAL 10)
SET(VHOST_RESTART_FLAG 1)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/service-kikimr.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/vhost.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/vhost-endpoint.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/storage/core/tests/recipes/qemu.inc)

END()
