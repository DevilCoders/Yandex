from typing import Union

from yc_common import config
from yc_common import logging
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.lib import TaskUtils
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.queue.yc_marketplace_queue.exceptions import TaskParamValidationError
from cloud.marketplace.queue.yc_marketplace_queue.models.params import ExportTableLbParams
from cloud.marketplace.queue.yc_marketplace_queue.models.params import ExportTablesParams
from cloud.marketplace.queue.yc_marketplace_queue.types.export import ExportDestinationType
from cloud.marketplace.queue.yc_marketplace_queue.utils.checks import get_params

log = logging.get_logger("yc_marketplace_queue")

DESTINATION_TO_TASKS = {
    ExportDestinationType.LOGBROKER: "export_table_to_lb",
}


def task(params: dict, *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        export_params = get_params(params["params"], ExportTablesParams)
    except TaskParamValidationError as e:
        return e.resolution_resolved

    count = 0
    tables = config.get_value("{}_export.tables".format(export_params.export_destination))
    with marketplace_db().transaction() as tx:
        for count, table in enumerate(tables, start=1):
            TaskUtils.create(
                operation_type=DESTINATION_TO_TASKS[export_params.export_destination],
                params=ExportTableLbParams({"table_name": table}).to_primitive(),
                tx=tx,
            )

    log.debug("Created %s tasks for export.", count)

    return TaskResolution.resolve(status=TaskResolution.Status.RESOLVED)
