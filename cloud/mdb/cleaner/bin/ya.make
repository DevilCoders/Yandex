PY3_PROGRAM(dbaas-cleaner)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/cleaner/internal
)

PY_SRCS(dbaas_cleaner.py)

PY_MAIN(cloud.mdb.cleaner.bin.dbaas_cleaner:_main)

END()
