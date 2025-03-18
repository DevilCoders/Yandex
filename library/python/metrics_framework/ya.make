PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(0.0.5)

PEERDIR(
    contrib/python/django/django-1.11
    library/python/ylock
    library/python/yenv
    library/python/statface_client
)

PY_SRCS(
    TOP_LEVEL
    metrics_framework/__init__.py
    metrics_framework/admin.py
    metrics_framework/decorators.py
    metrics_framework/lock.py
    metrics_framework/models.py
    metrics_framework/settings.py
    metrics_framework/tasks.py
    metrics_framework/management/__init__.py
    metrics_framework/management/commands/__init__.py
    metrics_framework/management/commands/start_metrics_tasks.py
)

END()

RECURSE(
    metrics_framework/migrations
)
