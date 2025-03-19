from yc_common import config
from yc_common import logging
from yc_common.clients.kikimr import retry_idempotent_kikimr_errors
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.misc import timestamp
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.db.models import tasks_table
from cloud.marketplace.common.yc_marketplace_common.models.task import Task

log = logging.get_logger("yc_marketplace_queue")


@retry_idempotent_kikimr_errors
def _process(item):
    log.debug("Process task: {}".format(item.id))
    ids = SqlIn("id", item.depends)
    if tasks_table.client.select_one("SELECT * FROM $table WHERE ? AND NOT done LIMIT 1", ids, model=Task) is None:
        with marketplace_db().with_table_scope(tasks_table).transaction() as tx:
            tx.update_object("UPDATE $table $set WHERE id = ?", {
                "can_do": True,
                "modified_at": timestamp(),
            }, item.id)
        log.debug("Processed task: {}".format(item.id))
    else:
        log.debug("Deps not resolved for task: {}".format(item.id))


def _do(offset):
    log.debug("Do bucket with offset: {}".format(offset))
    limit = config.get_value("queue.get_deps_page_limit")

    query = "SELECT * FROM $table WHERE NOT can_do ORDER BY created_at DESC LIMIT ? OFFSET ?"
    tasks = tasks_table.client.select(query, limit, offset, model=Task)

    g = 0
    for item in tasks:
        if g == 0:
            _do(limit + offset)
        g += 1
        _process(item)


def task(*args, **kwargs):
    log.debug("Start task for resolve deps")
    _do(0)
    log.debug("Finish task for resolve deps")

    return None
