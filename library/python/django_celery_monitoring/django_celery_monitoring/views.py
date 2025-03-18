# coding: utf-8
from __future__ import unicode_literals

from celery import states
from django.http import JsonResponse
from django.views.generic.base import View
from django_celery_results.models import TaskResult

from django_celery_monitoring.utils import get_muted_task_names


class MonitoringCeleryView(View):
    """
    Ручка мониторинга celery-задач
    """
    def get(self, request, *args, **kwargs):
        task_names = list(
            TaskResult.objects
            .filter(status=request.GET.get('status', states.FAILURE))
            .exclude(task_name__in=get_muted_task_names())
            .order_by('task_name')
            .values_list('task_name', flat=True)
            .distinct()
        )

        if not task_names:
            return JsonResponse({})

        return JsonResponse({'task_names': task_names}, status=400)
