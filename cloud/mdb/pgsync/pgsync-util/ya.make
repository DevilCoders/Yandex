OWNER(g:mdb)

PY3_PROGRAM(pgsync-util)

STYLE_PYTHON()

PY_MAIN(cloud.mdb.pgsync.cli:entry)

PEERDIR(
    cloud/mdb/pgsync
)

END()
