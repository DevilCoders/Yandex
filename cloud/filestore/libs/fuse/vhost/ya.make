LIBRARY()

OWNER(g:cloud-nbs)

CFLAGS(
    -DFUSE_VIRTIO
    -DFUSE_USE_VERSION=31
)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/libs/fuse/sources.inc)

SRCDIR(cloud/filestore/libs/fuse)

SRCS(
    # virtio-fs glue for FUSE
    vhost/fuse_virtio.c
)

PEERDIR(
    cloud/contrib/vhost
    contrib/libs/virtiofsd
)

END()
