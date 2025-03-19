PY3_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    contrib/python/django/django-3

    library/python/django
)

PY_SRCS(
    app.py
    urls.py
    lock.py
    views.py
    filters.py
    templatetags/mlock/templatetags.py
)

RESOURCE_FILES(
    cloud/mdb/backstage/apps/mlock/templates/mlock/locks.html
)

END()
