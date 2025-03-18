# coding: utf-8
from __future__ import unicode_literals

import celery.states

from django.contrib import admin, messages
from django.http import HttpResponseRedirect
from django.urls import reverse
from django.utils.html import format_html
from django_celery_results.apps import CeleryResultConfig
from django_celery_results.models import TaskResult
from django_celery_results.admin import TaskResultAdmin as TaskResultAdminBase

from django_celery_monitoring import settings
from django_celery_monitoring.models import TaskResultExtra
from django_celery_monitoring.utils import (
    get_task_signature_from_result,
    mute_tasks,
    unmute_tasks,
    get_muted_task_names,
)

CELERY_STATUS_MUTED = 'MUTED'
CELERY_FAILURE_STATUSES = (
    celery.states.FAILURE,
    CELERY_STATUS_MUTED,
)

# Note: костылище – добавляем новый статус в модель TaskResult
TaskResult._meta.get_field('status').choices.append((CELERY_STATUS_MUTED, CELERY_STATUS_MUTED))


CELERY_RESULTS_APP_LABEL = CeleryResultConfig.label


def format_url(url, name=None, title=None):
    name = name or url
    title = title or name
    return format_html(
        '<a target="_blank" href="{url}" title="{title}">{name}</a>',
        url=url,
        name=name,
        title=title,
    )


class TaskNameFilter(admin.SimpleListFilter):

    title = 'Task Name'
    parameter_name = 'task_name'

    def get_task_names(self):
        return list(
            TaskResult.objects
            .values_list('task_name', flat=True)
            .order_by('task_name')
            .distinct()
        )

    def lookups(self, request, model_admin):
        display = settings.CELERY_MONITORING_ADMIN_TASK_NAME_DISPLAY
        return sorted((i, display(i)) for i in self.get_task_names())

    def queryset(self, request, queryset):
        if not self.value():
            return queryset
        return queryset.filter(task_name=self.value())


class HideMutedTaskFilter(admin.SimpleListFilter):
    """
    Фильтр для скрытия замьюченных тасков
    """
    title = 'Hide muted tasks'
    parameter_name = 'hide_muted_task'

    def lookups(self, request, model_admin):
        return [('true', 'Да')]

    def queryset(self, request, queryset):
        if not self.value():
            return queryset
        return queryset.exclude(task_name__in=get_muted_task_names())


class MutedTaskFilter(TaskNameFilter):
    """
    Фильтр нужен больше для того,
    чтобы легко отобразить все замьюченные таски в админке.
    """
    title = 'Muted tasks'
    parameter_name = 'muted_task_name'

    def get_task_names(self):
        return get_muted_task_names()


class TaskResultExtraInlineAdmin(admin.StackedInline):

    model = TaskResultExtra


class TaskResultAdmin(TaskResultAdminBase):

    actions = TaskResultAdminBase.actions + [
        'mute_one',
        'mute',
        'unmute',
        'restart',
    ]

    list_display = (
        'task_id',
        'help',
        'date_done',
        'status',
    )

    list_filter = (
        MutedTaskFilter,
        HideMutedTaskFilter,
        'status',
        'date_done',
        TaskNameFilter,
    )

    inlines = (TaskResultExtraInlineAdmin,)

    def help(self, obj):
        if not settings.CELERY_MONITORING_WIKI_URL:
            return obj.task_name
        # wiki удаляет из url'ов знак `_`
        url_hash = obj.task_name.replace('_', '')
        return format_url(
            url='{}#{}'.format(settings.CELERY_MONITORING_WIKI_URL, url_hash),
            name=obj.task_name,
            title='Что делать?',
        )

    def mute_one(self, request, queryset):
        """
        Замьютить один конкретный зафейлившийся task result.
        Т.е. просто перевести его в статус MUTED,
        чтобы он и в фейлах не светился, и не удалялся автоматом
        """
        if queryset.exclude(status__in=CELERY_FAILURE_STATUSES).exists():
            messages.error(
                request=request,
                message='Выполнимо только для статусов: %s' % ', '.join(CELERY_FAILURE_STATUSES),
            )
            return

        queryset.update(status=CELERY_STATUS_MUTED)
        messages.success(
            request=request,
            message='Таски больше не мониторятся',
        )

    def mute(self, request, queryset):
        task_names = set(queryset.values_list('task_name', flat=True))
        mute_tasks(task_names)
        messages.success(
            request=request,
            message='Следующие таски больше не мониторятся: %s' % ', '.join(task_names),
        )

    def unmute(self, request, queryset):
        task_names = set(queryset.values_list('task_name', flat=True))
        unmute_tasks(task_names)
        messages.success(
            request=request,
            message='Следующие таски снова мониторятся: %s' % ', '.join(task_names),
        )

    def restart(self, request, queryset):
        if queryset.exclude(status__in=CELERY_FAILURE_STATUSES).exists():
            messages.error(
                request=request,
                message='Выполнимо только для статусов: %s' % ', '.join(CELERY_FAILURE_STATUSES),
            )
            return

        success_ids = []

        for obj in queryset:
            try:
                task = get_task_signature_from_result(obj)
            except (ValueError, SyntaxError):
                task_url = reverse(
                    'admin:{}_taskresult_change'.format(CELERY_RESULTS_APP_LABEL),
                    args=[obj.id],
                )
                messages.error(
                    request=request,
                    message=format_url(
                        url=task_url,
                        name='Таск %s: не удалось распарсить args/kwargs' % obj.task_id,
                    ),
                )
            else:
                task.delay()
                success_ids.append(obj.id)
                if settings.CELERY_MONITORING_ENABLE_ADMIN_LOGGING:
                    message = 'Restarted task {}[{}]'.format(obj.task_id, obj.task_name)
                    self.log_change(request, obj, message)

        if success_ids:
            messages.success(request, 'Было перезапущено тасков: %d' % len(success_ids))
            url = '{url}?id__in={ids}'.format(
                url=reverse('admin:{}_taskresult_changelist'.format(CELERY_RESULTS_APP_LABEL)),
                ids=','.join(str(i) for i in success_ids),
            )
            return HttpResponseRedirect(url)
        else:
            messages.warning(request, 'Ни один таск не перезапущен')

    mute_one.short_description = 'Скрыть с мониторингов'
    mute.short_description = 'Скрыть с мониторингов все таски такого типа'
    unmute.short_description = 'Показывать на мониторингах все таски такого типа'
    restart.short_description = 'Перезапустить'


admin.site.unregister(TaskResult)
admin.site.register(TaskResult, TaskResultAdmin)
