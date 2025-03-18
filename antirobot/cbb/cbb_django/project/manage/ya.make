PY3_PROGRAM(manage)

OWNER(g:antirobot)

PEERDIR(
    library/python/django
    antirobot/cbb/cbb_django/project
)

PY_SRCS(
    manage.py
)

PY_MAIN(antirobot.cbb.cbb_django.project.manage.manage)
NO_CHECK_IMPORTS()
END()

