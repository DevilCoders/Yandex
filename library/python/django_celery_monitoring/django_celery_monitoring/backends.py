# coding: utf-8
from __future__ import unicode_literals

from django_celery_results.backends.database import DatabaseBackend
from django_celery_results.managers import transaction_retry

from django_celery_monitoring.models import TaskResultExtra


class DatabaseExtendedBackend(DatabaseBackend):
    """
    Расширенный django-db бэкенд.
    Дополнительно сохраняет следующие данные:
    - chain

    Чтобы включить его, в конфигах нужно задать:
    CELERY_RESULT_BACKEND = 'django-db-ext'
    """
    def _store_task_result(self, task_id, result, status, traceback=None, request=None, using=None):
        content_type, content_encoding, result = self.encode_content(result)
        _, _, meta = self.encode_content({
            'children': self.current_task_children(request),
        })

        task_name = getattr(request, 'task', None) if request else None

        # Note: в django-celery-results вместо аргументов сохраняется (kw)argsrepr.
        # Это позволяет не сохранять чувствительные данные в БД.
        # Для django-celery-monitoring такой подход не работает, потому что перезапустить такой
        # таск зачастую невозможно, из-за того что аргументы обрезаются.
        task_args = getattr(request, 'args', None) if request else None
        task_kwargs = getattr(request, 'kwargs', None) if request else None

        self.TaskModel._default_manager.store_result(
            content_type=content_type,
            content_encoding=content_encoding,
            task_id=task_id,
            result=result,
            status=status,
            traceback=traceback,
            meta=meta,
            task_name=task_name,
            task_args=task_args,
            task_kwargs=task_kwargs,
            using=using,
        )
        return result

    @transaction_retry(max_retries=2)
    def _store_task_extra(self, task_id, request):
        defaults = dict(
            task_chain=getattr(request, 'cached_chain', []),
        )
        extra, created = TaskResultExtra.objects.get_or_create(
            task_result_id=task_id,
            defaults=defaults,
        )
        if not created:
            for field, value in defaults.items():
                setattr(extra, field, value)
            extra.save(update_fields=defaults.keys())

    def _store_result(self, task_id, result, status, traceback=None, request=None, using=None):
        self._store_task_result(task_id, result, status, traceback, request, using)
        self._store_task_extra(task_id, request)
        return result
