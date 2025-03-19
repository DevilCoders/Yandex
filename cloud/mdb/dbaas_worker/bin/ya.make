PY3_PROGRAM(dbaas-worker)

STYLE_PYTHON()

OWNER(g:mdb)

PY_MAIN(cloud.mdb.dbaas_worker.bin:main)

ALL_PY_SRCS()

PEERDIR(
    cloud/mdb/dbaas_worker/internal
    cloud/mdb/internal/python/ipython_repl
)

END()
