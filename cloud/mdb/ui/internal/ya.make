PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

FORK_TESTS()

RESOURCE_FILES(
    PREFIX cloud/mdb/ui/internal
    #    django/contrib/admin/templates/admin/base.html
    #    admin_tools/theming/templates/admin/base.html
)

PEERDIR(
    cloud/mdb/ui/deps
    contrib/python/psycopg2
    contrib/python/django/django-2.2
    contrib/python/pytils
    contrib/python/django-nested-inline
    contrib/python/django-postgrespool2
    contrib/python/sqlalchemy/sqlalchemy-1.4
    contrib/python/django-model-choices
    contrib/python/django-object-actions
    contrib/python/webencodings
    contrib/python/django-js-asset
    contrib/python/django-jinja
    contrib/python/django-csp
    contrib/python/uwsgi
    contrib/python/ipython
    contrib/python/sentry-sdk
    contrib/python/sentry-sdk/sentry_sdk/integrations/django
    contrib/python/django-admin-inline-paginator
    library/python/python-blackboxer
    library/python/ylog
    library/python/tvm2
    library/python/django
)

ALL_PY_SRCS(
    RECURSIVE
    NAMESPACE
    cloud.mdb.ui.internal
)

RESOURCE_FILES(
    PREFIX cloud/mdb/ui/internal/
    config.ini
)

NO_CHECK_IMPORTS(
    django_jinja.contrib.*
    django_postgrespool2.*
)

INCLUDE(ya.resources.static)

END()
