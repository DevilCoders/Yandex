LIBRARY()

OWNER(g:cloud-nbs)

CFLAGS(
    -DFUSE_VIRTIO
    -DFUSE_USE_VERSION=31
)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/libs/fuse/sources.inc)

SRCDIR(cloud/filestore/libs/fuse)

SRCS(
    test/context.cpp
    test/request.cpp

    # virtio-fs glue for FUSE
    test/fuse_virtio.c
)

PEERDIR(
    contrib/libs/virtiofsd
)

END()
