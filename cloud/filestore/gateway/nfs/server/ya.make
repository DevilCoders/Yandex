PROGRAM(filestore-nfs)

OWNER(g:cloud-nbs)

ALLOCATOR(TCMALLOC_TC)

SPLIT_DWARF()

CFLAGS(
    -Wno-unused-parameter
    -Wno-unused-variable
)

PEERDIR(
    cloud/filestore/gateway/nfs/libs/api
    cloud/filestore/gateway/nfs/libs/fsal
    cloud/filestore/gateway/nfs/libs/sal
    cloud/filestore/gateway/nfs/libs/service

    cloud/filestore/libs/client

    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics

    library/cpp/lwtrace/mon

    library/cpp/deprecated/atomic
    library/cpp/getopt
    library/cpp/logger
    library/cpp/sighandler

    contrib/restricted/nfs_ganesha/src/FSAL/FSAL_MEM
    contrib/restricted/nfs_ganesha/src/FSAL/FSAL_VFS
    contrib/restricted/nfs_ganesha/src/MainNFSD
)

ADDINCL(
    contrib/restricted/libntirpc
    contrib/restricted/libntirpc/ntirpc
    contrib/restricted/liburcu/include
    contrib/restricted/nfs_ganesha/src/include
)

SRCS(
    app.cpp
    bootstrap.cpp
    ganesha.c
    main.cpp
    options.cpp
)

END()
