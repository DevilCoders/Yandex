PY23_LIBRARY()

OWNER(g:python-contrib)

VERSION(0.7.0)

PEERDIR(
    contrib/python/six
    contrib/python/django-appconf
    contrib/python/psycopg2
)

PY_SRCS(
    TOP_LEVEL
    django_pgaas/compat.py
    django_pgaas/transaction.py
    django_pgaas/conf.py
    django_pgaas/management/__init__.py
    django_pgaas/management/commands/db.py
    django_pgaas/management/commands/__init__.py
    django_pgaas/__init__.py
    django_pgaas/hosts.py
    django_pgaas/backend/compiler.py
    django_pgaas/backend/__init__.py
    django_pgaas/backend/operations.py
    django_pgaas/backend/base.py
    django_pgaas/utils.py
    django_pgaas/wsgi.py
)

END()
