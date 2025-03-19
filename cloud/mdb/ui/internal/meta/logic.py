from django.db import connections
from django.conf import settings
from .models import WorkerQueue


def can_restart_task(task: WorkerQueue) -> bool:
    if task.result or task.result is None:
        return False

    if task.restart_count > 1:  # type: ignore
        return False

    if task.task_type not in settings.RESTART_TASKS_WHITE_LIST:
        return False

    with connections['meta_slave'].cursor() as cursor:
        cursor.execute(
            """SELECT 1
            FROM code.cluster_status_acquire_transitions()
            WHERE from_status = %(cluster_status)s AND action = code.task_type_action(%(task_type)s)
        """,
            {'cluster_status': task.cid.status, 'task_type': task.task_type},
        )
        results = cursor.fetchone()
    if not results:
        return False  # invalid cluster status

    return True


def restart_task(task_id: str) -> None:
    with connections['meta_primary'].cursor() as cursor:
        cursor.execute('SELECT code.restart_task(%(task_id)s)', {'task_id': task_id})
