LIBRARY()

OWNER(g:cloud-nbs)

CFLAGS(
    -Wno-unused-parameter
    -Wno-unused-variable
)

PEERDIR(
    cloud/filestore/gateway/nfs/libs/api

    contrib/restricted/nfs_ganesha/src/MainNFSD
)

ADDINCL(
    contrib/restricted/libntirpc
    contrib/restricted/libntirpc/ntirpc
    contrib/restricted/liburcu/include
    contrib/restricted/nfs_ganesha/src/include
)

SRCS(
    GLOBAL main.c
    recovery.c
)

END()
