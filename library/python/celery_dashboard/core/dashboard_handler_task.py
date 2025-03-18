from functools import cached_property

import celery

from .redis_dashboard_io import DashboardIO
from library.python.celery_dashboard.core.task_context import TaskContext, TaskStatus


class DashboardHandlerTask(celery.Task):
    abstract = True

    @cached_property
    def dashboard_io(self):
        return DashboardIO()

    def run(self, *args, **kwargs):
        super(DashboardHandlerTask, self).run(*args, **kwargs)

    def _make_task_context(self, task_id, task_args, task_kwargs, status, **kwargs):
        return TaskContext(
            name=self.name,
            id=task_id,
            status=status,
            args=task_args,
            kwargs=task_kwargs,
            extra=kwargs
        )

    def on_success(self, retval, task_id, args, kwargs):
        task_context = self._make_task_context(task_id, args, kwargs, status=TaskStatus.SUCCESS)
        self.dashboard_io.store_success_task(task_context)

    def on_retry(self, exc, task_id, args, kwargs, einfo):
        task_context = self._make_task_context(task_id, args, kwargs, status=TaskStatus.RETRY, exc=exc, einfo=einfo)
        self.dashboard_io.store_retried_task(task_context)

    def on_failure(self, exc, task_id, args, kwargs, einfo):
        task_context = self._make_task_context(task_id, args, kwargs, status=TaskStatus.FAILED, exc=exc, einfo=einfo)
        self.dashboard_io.store_failed_task(task_context)
