PY3_PROGRAM()

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/libs/storage/protos
    cloud/blockstore/tools/cms/lib
)


END()
