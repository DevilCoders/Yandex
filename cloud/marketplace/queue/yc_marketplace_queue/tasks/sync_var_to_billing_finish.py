from typing import List
from typing import Union

from yc_common import config
from yc_common import logging
from cloud.marketplace.common.yc_marketplace_common.client.billing import BillingPrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token

log = logging.get_logger("yc_marketplace_queue")


def task(depends: List[Task], params, *args, **kwargs) -> Union[TaskResolution, None]:
    start_task = depends[0]
    billing_operation_id = start_task.response["operation_id"]
    op = BillingPrivateClient(
        billing_url=config.get_value("endpoints.billing.url"),
        iam_token=metadata_token.get_instance_metadata_token(),
    ).get_operation(billing_operation_id)

    if op.done:
        if op.response:
            return TaskResolution.resolve(
                status=TaskResolution.Status.RESOLVED, data={},
            )
        return TaskResolution.resolve(status=TaskResolution.Status.FAILED, data={"error": op.error})

    return None
