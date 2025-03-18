PY3_LIBRARY()

OWNER(g:tools-python)

VERSION(0.5.2)

PEERDIR(
    contrib/python/celery/py2
    contrib/deprecated/python/django-celery-results
)

PY_SRCS(
    TOP_LEVEL
    django_celery_monitoring/__init__.py
    django_celery_monitoring/admin.py
    django_celery_monitoring/apps.py
    django_celery_monitoring/backends.py
    django_celery_monitoring/mixins.py
    django_celery_monitoring/models.py
    django_celery_monitoring/settings.py
    django_celery_monitoring/utils.py
    django_celery_monitoring/views.py
    django_celery_monitoring/migrations/__init__.py
    django_celery_monitoring/migrations/0001_initial.py
    django_celery_monitoring/migrations/0002_taskresultextra.py
)

END()
