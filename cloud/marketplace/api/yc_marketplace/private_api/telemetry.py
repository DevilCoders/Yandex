import logging

from yc_common import metrics
from yc_common.misc import timestamp
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.metrics import collect
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.db.models import tasks_table

task_queue_length = metrics.Metric(
    metrics.MetricTypes.GAUGE,
    "task_queue_length",
    ["type"],
    "Task queue length gauge.")

task_exec_time = metrics.Metric(
    metrics.MetricTypes.GAUGE,
    "task_exec_time",
    ["type", "status", "quantile"],
    "Task execution time histogram.")

MINUTE_SECONDS = 60

log = logging.getLogger(__name__)


def get_queue_length_total():
    query = """SELECT id as cnt FROM $table
               WHERE NOT done AND operation_type != 'resolve_dependencies' LIMIT 1000"""
    query_result = marketplace_db().with_table_scope(tasks_table).select(query, commit=True)

    return {"pending": len(query_result)}


def get_execution_time(offset=10 * MINUTE_SECONDS):
    query = """SELECT PERCENTILE(t.execution_time_ms, 0.5) AS q50,
                      PERCENTILE(t.execution_time_ms, 0.7) AS q70,
                      PERCENTILE(t.execution_time_ms, 0.9) AS q90,
                      PERCENTILE(t.execution_time_ms, 0.99) AS q99,
                      t.operation_type
               FROM $table as t
               WHERE t.modified_at >= ?
               AND done
               GROUP BY t.operation_type"""

    operations = marketplace_db().with_table_scope(tasks_table).select(
        query, timestamp() - offset, strip_table_from_columns=True, commit=True)

    return list(operations)


@private_api_handler("GET", "/telemetry")
def get_telemetry(**kwargs):
    metrics.reset(task_queue_length.name)
    try:
        queue_length = get_queue_length_total()
        for task_status, task_count in queue_length.items():
            task_queue_length.labels(task_status).set(task_count)
    except Exception as e:
        log.error("Failed to collect task queue length: %s", e)

    metrics.reset(task_exec_time.name)
    try:
        for op in get_execution_time():
            for q in ("q50", "q70", "q90", "q99"):
                task_exec_time.labels(op["operation_type"], op["status"], q).set(op[q])
    except Exception as e:
        log.error("Failed to collect task execution time: %s", e)

    return collect()
