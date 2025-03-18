PY3_LIBRARY(antirobot.cbb.cbb_django)

OWNER(g:antirobot)

PEERDIR(
    contrib/python/django/django-2.2
    contrib/python/gunicorn
    library/python/django

    devtools/ya/yalibrary/makelists

    antirobot/cbb/cbb_django/cbb

    contrib/python/python-memcached

    library/python/python-django-yauth
    library/python/django-idm-api
)

PY_SRCS(
    settings.py
    urls.py
)

END()

