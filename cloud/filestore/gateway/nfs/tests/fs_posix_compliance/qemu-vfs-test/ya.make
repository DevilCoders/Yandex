PY3TEST()

OWNER(g:cloud-nbs)

SIZE(LARGE)
TIMEOUT(600)

FORK_SUBTESTS()

REQUIREMENTS(
    container:2273379275
)

TAG(ya:fat ya:force_sandbox ya:privileged)

# requires root
TAG(ya:manual)

PEERDIR(
    cloud/filestore/tests/python/lib
)

TEST_SRCS(
    test.py
)

SET(QEMU_VIRTIO nfs)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tools/testing/fs_posix_compliance/fs_posix_compliance.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/nfs-ganesha-vfs.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/storage/core/tests/recipes/qemu.inc)

END()
