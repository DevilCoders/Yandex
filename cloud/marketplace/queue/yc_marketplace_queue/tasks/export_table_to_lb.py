from typing import Union

from cloud.billing.lb_exporter import core as yc_lb_exporter
from cloud.marketplace.common.yc_marketplace_common.db.models import DB_LIST_BY_NAME
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.queue.yc_marketplace_queue.config import LogbrokerConfig
from cloud.marketplace.queue.yc_marketplace_queue.exceptions import TableNotFoundError
from cloud.marketplace.queue.yc_marketplace_queue.exceptions import TaskParamValidationError
from cloud.marketplace.queue.yc_marketplace_queue.models.params import ExportTableLbParams
from cloud.marketplace.queue.yc_marketplace_queue.utils.checks import get_params
from kikimr.public.sdk.python.persqueue.auth import TVMCredentialsProvider
import ticket_parser2 as tp2
from yc_common import config
from yc_common import logging
from yc_common.misc import timestamp

log = logging.get_logger("yc_marketplace_queue")


def _get_table_by_name(table_name):
    if table_name not in DB_LIST_BY_NAME:
        raise TableNotFoundError(
            resolution_resolved=TaskResolution.resolve(
                status=TaskResolution.Status.FAILED,
                data={
                    "code": "TableNotFoundError",
                    "message": "Table `{}` not found in DB's list.".format(table_name),
                }),
        )

    return DB_LIST_BY_NAME[table_name]


def _export(table):
    logbroker_cfg = config.get_value("endpoints.logbroker", model=LogbrokerConfig)

    tvm_config = logbroker_cfg.auth

    tvm_settings = tp2.TvmApiClientSettings(
        self_client_id=tvm_config.client_id,
        self_secret=tvm_config.secret,
        dsts={"pq": tvm_config.destination},
    )

    tvm_auth = TVMCredentialsProvider(
        tvm_client=tp2.TvmClient(tvm_settings),
        destination_client_id=tvm_config.destination,
        destination_alias="pq",
    )

    yc_lb_exporter.export(
        lb_host=logbroker_cfg.host,
        lb_port=logbroker_cfg.port,
        lb_topic_name=logbroker_cfg.topic_template.format(subject=table.name),
        lb_auth=tvm_auth,
        table=table,
        extra_export_info={
            "exported_at": timestamp(),
        },
        log=log,
    )


def task(params: dict, *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        export_params = get_params(params["params"], ExportTableLbParams)
        table = _get_table_by_name(export_params.table_name)
    except (TaskParamValidationError, TableNotFoundError) as e:
        return e.resolution_resolved

    _export(table)

    return TaskResolution.resolve(status=TaskResolution.Status.RESOLVED)
