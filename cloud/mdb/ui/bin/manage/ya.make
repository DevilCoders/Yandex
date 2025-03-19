PY3_PROGRAM(manage)

STYLE_PYTHON()

OWNER(g:mdb)

ENV(DJANGO_SETTINGS_MODULE=cloud.mdb.ui.internal.mdbui.settings)

PEERDIR(
    cloud/mdb/ui/internal
)

PY_SRCS(app.py)

PY_MAIN(cloud.mdb.ui.internal.manage:django_main)

NO_CHECK_IMPORTS()

END()
