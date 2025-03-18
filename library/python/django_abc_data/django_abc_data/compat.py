from __future__ import unicode_literals

try:
    from django.utils.module_loading import import_string
except ImportError:
    from django.utils.module_loading import import_by_path as import_string

try:
    from celery import shared_task
except ImportError:
    try:
        from celery.decorators import task as shared_task
    except ImportError:
        shared_task = NotImplemented  # in case we do not want to use celery


__all__ = ('import_string', 'shared_task')
