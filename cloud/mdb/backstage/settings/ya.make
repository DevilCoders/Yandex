PY3_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    contrib/python/tzdata
    contrib/python/psycopg2
    contrib/python/django/django-3
    contrib/python/django-crispy-forms

    library/python/django
    library/python/python-django-yauth
    library/python/tvmauth

    metrika/pylib/config

    cloud/mdb/backstage/lib
    cloud/mdb/backstage/apps/iam
    cloud/mdb/backstage/apps/main
    cloud/mdb/backstage/apps/main/migrations
    cloud/mdb/backstage/apps/cms
    cloud/mdb/backstage/apps/dbm
    cloud/mdb/backstage/apps/meta
    cloud/mdb/backstage/apps/katan
    cloud/mdb/backstage/apps/deploy
    cloud/mdb/backstage/apps/mlock
)

PY_SRCS(
    settings.py
)

RESOURCE(
    cloud/mdb/backstage/settings/config.yaml config.yaml
)

NO_CHECK_IMPORTS()

END()
