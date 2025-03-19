import os
import sys
import traceback
from typing import Callable
from typing import List  # noqa

from yc_common import config
from yc_common import logging
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.misc import timestamp
from yc_common.misc import timestamp_ms
from cloud.marketplace.common.yc_marketplace_common.db.models import tasks_table
from cloud.marketplace.common.yc_marketplace_common.lib import TaskUtils
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.queue.yc_marketplace_queue import tasks

log = logging.get_logger("yc_marketplace_queue")
hostname = os.getenv("HOSTNAME")


def get_task(operation_type: str) -> Callable[..., TaskResolution]:
    return getattr(tasks, operation_type)


def do(uid, skip):
    task = TaskUtils.lock(uid, hostname, skip)
    if task is None:
        return

    # run task
    try:
        task_func = get_task(task.operation_type)
    except AttributeError:
        traceback.print_exc(file=sys.stdout)
        log.error("Method not found!")
        resolution = TaskResolution.resolve(TaskResolution.Status.NOTIMPLEMENTED)
        execution_time_ms = 0

    else:
        log.debug("Collect dependency: {}".format(uid))
        if task.depends is not None and len(task.depends) > 0:
            ids = task.depends
            # Need to covert to list because select return one time generator
            depends = list(tasks_table.client.select("SELECT * FROM $table WHERE ?", SqlIn("id", ids),
                                                     model=Task))  # type: List[Task]
        else:
            depends = []

        if any(d.error for d in depends):
            resolution = TaskResolution.resolve(
                status=TaskResolution.Status.FAILED,
                data={
                    "code": "TaskDepFailed",
                    "message": "Dependent task failed",
                })
            execution_time_ms = 0
        else:
            log.info("Running: {}".format(uid))

            kwargs = {
                "id": task.id,
                "operation_type": task.operation_type,
                "params": task.params,
                "depends": depends,
            }
            start = timestamp_ms()
            resolution = task_func(**kwargs)
            execution_time_ms = timestamp_ms() - start

    log.info("Trying write task result: {}, {}, {}".format(uid, resolution, execution_time_ms))
    if resolution is None:
        return

    if not task.params.is_infinite:
        TaskUtils.finish(task.id, task.lock, resolution, execution_time_ms)
    log.info("Done task: {}, {}".format(uid, resolution))


def get_tasks():
    limit = config.get_value("queue.count")
    query = """SELECT *
               FROM $table
               WHERE NOT done
               AND can_do
               AND unlock_after < ?
               AND kind = ?
               LIMIT ?"""

    t = tasks_table.client.select(query, timestamp(), Task.Kind.INTERNAL, limit, model=Task)
    log.debug('get_tasks: {}'.format(t))
    return t


def main():
    default_skip = config.get_value("queue.timeout")
    for task in get_tasks():
        if task.params is None:
            skip = default_skip
        else:
            try:
                skip = int(task.params.get("params", {}).get('timeout', default_skip))
            except (AttributeError, ValueError, TypeError) as e:
                skip = default_skip

        log.info('Try to register async task {}'.format(task.id))
        do(task.id, skip)
        log.info('Registered async task {}'.format(task.id))
