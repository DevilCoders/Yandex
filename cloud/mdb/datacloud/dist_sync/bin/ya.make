PY3_PROGRAM(dist-sync)

STYLE_PYTHON()

OWNER(g:mdb)

PY_MAIN(cloud.mdb.datacloud.dist_sync.dist_sync:main)

PEERDIR(
    cloud/mdb/datacloud/dist_sync
)

END()
