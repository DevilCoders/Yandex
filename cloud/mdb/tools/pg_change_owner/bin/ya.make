PY3_PROGRAM(pg_change_owner)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/tools/pg_change_owner/internal
)

PY_SRCS(
    pg_change_owner.py
    logs.py
)

PY_MAIN(cloud.mdb.tools.pg_change_owner.bin.pg_change_owner:main)

END()
