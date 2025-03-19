PY3_PROGRAM(create_users)

STYLE_PYTHON()

OWNER(g:mdb)

PY_MAIN(cloud.mdb.internal.python.pg_create_users.bin.create_users:main)

PY_SRCS(create_users.py)

PEERDIR(
    cloud/mdb/internal/python/pg_create_users/internal
)

END()
