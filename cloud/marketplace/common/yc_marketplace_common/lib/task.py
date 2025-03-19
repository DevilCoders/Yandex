from typing import List
from typing import Optional
from typing import TypeVar
from typing import Union

from yc_common import logging
from yc_common.clients.kikimr.client import _KikimrBaseConnection
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.misc import timestamp
from yc_common.models import Model
from yc_common.paging import page_handler
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.db.models import tasks_table
from cloud.marketplace.common.yc_marketplace_common.models.operation import OperationList
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResponse
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidTaskIdError
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_order_by
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction

log = logging.get_logger(__name__)

Ob1 = TypeVar("Ob1", bound=OperationV1Beta1)


def _json_to_kikimr(field):
    if isinstance(field, Model):
        return field.to_db()
    return field


class TaskUtils:

    @classmethod
    @mkt_transaction()
    def create(cls,
               operation_type: str,
               params: dict, *,
               tx: _KikimrBaseConnection,
               group_id: str = None,
               is_infinite: bool = False,
               is_cancelable: bool = True,
               depends: List = None,
               **kwargs):
        if depends is None:
            depends = []
        tx = tx.with_table_scope(tasks=tasks_table)

        params = TaskParams({
            "params": params,
            "is_cancelable": is_cancelable,
            "is_infinite": is_infinite,
        })
        task = Task.new(id=None,
                        group_id=group_id,
                        operation_type=operation_type,
                        params=params,
                        done=False,
                        response=None,
                        error=None,
                        can_do=len(depends) == 0,
                        depends=depends,
                        **kwargs)
        tx.insert_object("UPSERT INTO $tasks_table", task)

        return task.to_operation_v1beta1()

    @classmethod
    @mkt_transaction()
    def fake(cls, op: Union[Ob1, dict], operation_type="None", *, tx) -> Ob1:
        tx = tx.with_table_scope(tasks=tasks_table)

        if isinstance(op, OperationV1Beta1):
            task = Task.new(
                id=None,
                group_id=None,
                operation_type=op.description or operation_type,
                metadata=_json_to_kikimr(op.metadata),
                done=True,
                response=_json_to_kikimr(op.response),
                error=_json_to_kikimr(op.error),
                can_do=False,
                depends=[],
                description=op.description,
                created_at=op.created_at,
                modified_at=op.modified_at,
                created_by=op.created_by,
            )
        else:
            task = Task.new(**op)

        tx.insert_object("UPSERT INTO $tasks_table", task)

        return task.to_operation_v1beta1()

    @classmethod
    @mkt_transaction()
    def get(cls, task_id, *, tx):
        task = tx.with_table_scope(tasks=tasks_table).select_one("SELECT * FROM $tasks_table WHERE id = ?", task_id,
                                                                 model=Task)

        if task is None:
            raise InvalidTaskIdError()

        return task

    @staticmethod
    @mkt_transaction()
    @page_handler(items="operations")
    def list(cursor, limit, filter_query, order_by=None, kind=Task.Kind.INTERNAL, *, tx=None):
        filter_query, filter_args = parse_filter(filter_query, Task)

        filter_query += ["kind = ?"]
        filter_args += [kind]

        filter_query = " AND ".join(filter_query)

        mapping = {
            "id": "id",
            "description": "description",
            "createdAt": "created_at",
            "modifiedAt": "modified_at",
            "createdBy": "created_by",
            "done": "done",
        }

        order_by = parse_order_by(order_by, mapping, "id")

        where_query, where_args = page_query_args(
            cursor,
            limit,
            id="id",
            filter_query=filter_query,
            filter_args=filter_args,
            order_by=order_by,
        )

        iterator = tx.with_table_scope(tasks_table).select(
            "SELECT * " +
            "FROM $table " + where_query,
            *where_args, model=Task)

        response = OperationList()

        for n, task in enumerate(iterator):
            # We assume that we have something else if we have reached the limit
            if limit is not None and n == limit - 1:
                response.next_page_token = task.id

            response.operations.append(task.to_operation_v1beta1())

        return response

    @staticmethod
    @mkt_transaction()
    @page_handler(items="operations")
    def group(cursor: str, limit: int, group_id: str, *, tx=None) -> OperationList:

        iterator = tx.with_table_scope(tasks_table).select(
            "SELECT * " +
            "FROM $table WHERE group_id = ?",
            group_id, model=Task)

        response = OperationList()

        for n, task in enumerate(iterator):
            # We assume that we have something else if we have reached the limit
            if limit is not None and n == limit - 1:
                response.next_page_token = task.id
            task.metadata = task.metadata or {}
            task.metadata["depends"] = task.depends
            response.operations.append(task.to_operation_v1beta1())

        return response

    @staticmethod
    def lock(task_id: str, hostname: str, skip_duration: int) -> Optional[Task]:
        log.debug("Try to lock task: {}".format(task_id))

        ts = timestamp()
        lock = generate_id()

        with marketplace_db().with_table_scope(tasks_table).transaction() as tx:
            tx.with_table_scope(tasks_table).update_object("""
                        UPDATE $table $set,
                        try_count = try_count + 1u
                        WHERE id = ? AND NOT done AND unlock_after < ?""", {
                "unlock_after": ts + skip_duration,
                "lock": lock,
                "worker_hostname": hostname,
                "modified_at": ts,
            }, task_id, ts)

        task = tasks_table.client.select_one(
            """SELECT {} FROM $table WHERE id = ? AND lock = ?""".format(Task.db_fields()),
            task_id, lock, model=TaskResponse)

        if task is None:
            log.debug("Can not lock: {}".format(task_id))
            return None
        else:
            log.debug("Task {} locked".format(task_id))
            return task

    @staticmethod
    @mkt_transaction()
    def finish(task_id: str, task_lock: str, resolution: TaskResolution, execution_time_ms: int, *, tx=None):
        resolution = resolution.to_kikimr()
        return tx.with_table_scope(tasks_table) \
            .update_object(
            "UPDATE $table $set WHERE NOT done AND id = ? AND lock = ?", {
                "done": True,
                "response": resolution["result"],
                "error": resolution["error"],
                "execution_time_ms": execution_time_ms,
                "modified_at": timestamp(),
            }, task_id, task_lock, commit=True)

    @staticmethod
    @mkt_transaction()
    def extend_lock(task_id: str, task_lock: str, interval: int, *, tx=None):
        ts = timestamp()
        tx.with_table_scope(tasks_table) \
            .update_object(
            "UPDATE $table $set WHERE NOT done AND id = ? AND lock = ?", {
                "unlock_after": ts + interval,
                "modified_at": ts,
            }, task_id, task_lock)
