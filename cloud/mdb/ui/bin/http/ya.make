PY3_PROGRAM(http)

STYLE_PYTHON()

OWNER(g:mdb)

ENV(DJANGO_SETTINGS_MODULE=cloud.mdb.ui.internal.mdbui.settings)

PEERDIR(
    cloud/mdb/ui/internal
    contrib/python/uwsgi
)

PY_SRCS(app.py)

PY_MAIN(pyuwsgi:run)

NO_CHECK_IMPORTS()

END()
