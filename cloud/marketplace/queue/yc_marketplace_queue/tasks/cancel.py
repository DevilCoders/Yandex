from typing import Union

from yc_common import logging
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.misc import timestamp
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.db.models import tasks_table
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution

log = logging.get_logger("yc_marketplace_queue")


def get_current_task(task_id: str) -> Task:
    return tasks_table.client.select_one("SELECT * FROM $table WHERE id = ?", task_id, model=Task)


def fetch_group_task_ids(current_task: Task) -> [str]:
    res = tasks_table.client.select("SELECT id, params FROM $table WHERE group_id = ? AND NOT done",
                                    current_task.group_id)
    # Get only cancelable tasks.
    return [row["id"] for row in res if row.get("params", {}).get("is_cancelable", True)]


def task(id: str, params: dict, **kwargs) -> Union[TaskResolution, None]:
    log.debug("Start cancel %s" % id)
    try:
        current_task = get_current_task(params["params"]["id"])
    except KeyError as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })

    try:
        ids = fetch_group_task_ids(current_task)
        log.debug("Continue cancel %s for %s" % (id, str(ids)))
        cancel_resolution = TaskResolution.resolve(status=TaskResolution.Status.CANCELED).to_kikimr()
        with marketplace_db().with_table_scope(tasks_table).transaction() as tx:
            tx.update_object("UPDATE $table $set WHERE ? AND NOT done ", {
                "done": True,
                "response": cancel_resolution["result"],
                "error": cancel_resolution["error"],
                "execution_time_ms": None,
                "modified_at": timestamp(),
            }, SqlIn("id", ids))
    except Exception:
        log.exception("Can not cancel task in %s", id)
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": "Can not cancel task in {}".format(id),
            })

    return TaskResolution.resolve(status=TaskResolution.Status.RESOLVED)
