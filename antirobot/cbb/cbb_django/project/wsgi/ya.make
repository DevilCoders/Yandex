PY3_PROGRAM(cbb)

OWNER(g:antirobot)

NO_CHECK_IMPORTS()

PEERDIR(
    library/python/django
    antirobot/cbb/cbb_django/project
    library/python/gunicorn
)

PY_SRCS(
    MAIN app.py
)

END()
