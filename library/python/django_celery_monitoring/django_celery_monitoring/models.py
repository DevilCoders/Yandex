# coding: utf-8
from __future__ import unicode_literals

from django.db import models


class MutedTask(models.Model):

    task_name = models.TextField(unique=True)


class TaskResultExtra(models.Model):
    """
    Дополнительная информация о TaskResult.
    """
    task_result = models.OneToOneField(
        to='django_celery_results.TaskResult',
        to_field='task_id',
        on_delete=models.CASCADE,
        related_name='extra',
    )
    task_chain = models.TextField()
