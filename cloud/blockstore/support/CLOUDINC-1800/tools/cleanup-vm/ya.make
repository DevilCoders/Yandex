PY3_PROGRAM()

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/tools/cms/lib

    devtools/ya/yalibrary/find_root
)

DEPENDS(
    cloud/blockstore/support/CLOUDINC-1800/tools/cleanup-ext4-meta
    cloud/blockstore/support/CLOUDINC-1800/tools/cleanup-fs
    cloud/blockstore/support/CLOUDINC-1800/tools/cleanup-xfs
    cloud/blockstore/support/CLOUDINC-1800/tools/copy_dev
)

END()
