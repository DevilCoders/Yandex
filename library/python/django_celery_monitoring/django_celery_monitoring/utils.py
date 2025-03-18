# coding: utf-8
from __future__ import unicode_literals

import ast
import logging
import importlib

from datetime import timedelta

from celery import states, chain
from django.core.exceptions import ObjectDoesNotExist
from django.utils import timezone
from django_celery_results.models import TaskResult

from django_celery_monitoring import settings
from django_celery_monitoring.models import MutedTask


logger = logging.getLogger(__name__)


def signature(name, *args, **kwargs):
    module_name, task_name = name.rsplit('.', 1)
    module = importlib.import_module(module_name)
    task = getattr(module, task_name)
    return task.signature(*args, **kwargs)


def _get_chain_from_result(task_result):
    try:
        extra = task_result.extra
    except ObjectDoesNotExist:
        return []

    raw_chain = ast.literal_eval(extra.task_chain)
    return [
        signature(
            t['task'],
            args=t['args'],
            kwargs=t['kwargs'],
            immutable=t.get('immutable'),
            task_id=t.get('options', {}).get('task_id')
        )
        for t in reversed(raw_chain)
    ]


def get_task_signature_from_result(task_result):
    """
    Превращает TaskResult в celery.Task,
    который можно запустить через .delay
    """
    main_signature = signature(
        task_result.task_name,
        task_id=task_result.task_id,
        args=ast.literal_eval(task_result.task_args),
        kwargs=ast.literal_eval(task_result.task_kwargs),
    )
    tasks_chain = _get_chain_from_result(task_result)
    if tasks_chain:
        return chain(main_signature, *tasks_chain)
    return main_signature


def get_muted_task_names():
    return set(MutedTask.objects.values_list('task_name', flat=True))


def mute_tasks(task_names):
    # TODO: тут можно использовать ignore conflicts, если django > 1.11
    tasks_to_mute = [
        MutedTask(task_name=task_name)
        for task_name in set(task_names) - get_muted_task_names()
    ]
    MutedTask.objects.bulk_create(tasks_to_mute)


def unmute_tasks(task_names):
    MutedTask.objects.filter(task_name__in=task_names).delete()


def clean_old_celery_results():
    dt = timezone.now() - timedelta(days=settings.CELERY_MONITORING_EXPIRE_DAYS)
    dt = dt.replace(hour=0, minute=0, second=0, microsecond=0)
    qs = TaskResult.objects.filter(
        status=states.SUCCESS,
        date_done__lt=dt,
    )
    qs.delete()


def fail_aborted_celery_results():
    """
    Переводит все таски, которые не завершились
    в течение суток, в статус FAILURE.
    Бывает такое, что выполнение таска прерывается
    по какой-то причине (например, при деплое).
    Спустя сутки мы можем считать их зафейлившимися.
    """
    dt = timezone.now() - timedelta(days=1)
    qs = TaskResult.objects.filter(
        date_done__lt=dt,
        status__in=(
            states.STARTED,
            states.RETRY,
        ),
    )
    qs.update(status=states.FAILURE, date_done=timezone.now())
