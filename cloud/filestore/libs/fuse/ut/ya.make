UNITTEST()

OWNER(g:cloud-nbs)

SRCDIR(cloud/filestore/libs/fuse)

SRCS(
    fs_ut.cpp
)

PEERDIR(
    cloud/filestore/libs/diagnostics
    cloud/filestore/libs/fuse/test
)

END()
