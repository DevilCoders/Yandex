PY3_PROGRAM(backstage-gunicorn)

OWNER(g:mdb)

PEERDIR(
    contrib/python/gunicorn

    metrika/pylib/config

    cloud/mdb/backstage/settings
)

PY_SRCS(
    __main__.py
)

NO_CHECK_IMPORTS()

END()
