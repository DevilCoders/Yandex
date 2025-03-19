PY3_PROGRAM(bootstrap.db.admin)

OWNER(g:ycselfhost)

PEERDIR(
    cloud/bootstrap/db/src/admin
)

PY_SRCS(
    __main__.py
)

END()
