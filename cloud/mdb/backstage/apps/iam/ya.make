PY3_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    library/python/django

    contrib/python/django/django-3

    metrika/pylib/http
    cloud/bitbucket/private-api/yandex/cloud/priv/oauth/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1

    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/logs
    cloud/mdb/internal/python/compute/iam/jwt
)

PY_SRCS(
    access_service.py
    app.py
    decorators.py
    lib.py
    jwt.py
    session_service.py
    middleware.py
    missing_ava.py
    urls.py
    user.py
    ajax.py
    views.py
    templatetags/iam/templatetags.py
)

RESOURCE_FILES(
    cloud/mdb/backstage/apps/iam/templates/iam/views/debug.html
    cloud/mdb/backstage/apps/iam/templates/iam/ajax/debug_config.html
    cloud/mdb/backstage/apps/iam/templates/iam/ajax/debug_auth_info.html
)

END()
