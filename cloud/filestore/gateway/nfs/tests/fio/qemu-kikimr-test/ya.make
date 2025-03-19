PY3TEST()

OWNER(g:cloud-nbs)

SIZE(LARGE)
TIMEOUT(600)

FORK_SUBTESTS()

REQUIREMENTS(
    ram:16
    container:2273379275
)

TAG(ya:fat ya:force_sandbox)

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

SET(QEMU_VIRTIO nfs)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/service-kikimr.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/nfs-ganesha.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/storage/core/tests/recipes/qemu.inc)

END()
