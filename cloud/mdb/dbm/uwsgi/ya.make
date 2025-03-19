PY3_PROGRAM(dbm.wsgi)

STYLE_PYTHON()

OWNER(g:mdb)

PY_MAIN(pyuwsgi:run)

PY_SRCS(application.py)

PEERDIR(
    contrib/python/uwsgi
    cloud/mdb/dbm/internal
)

END()
