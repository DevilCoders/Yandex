OWNER(g:mdb)

PY3_PROGRAM(gpsync-util)

STYLE_PYTHON()

PY_MAIN(cloud.mdb.gpsync.cli:entry)

PEERDIR(
    cloud/mdb/gpsync
)

END()
