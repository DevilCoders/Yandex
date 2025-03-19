from typing import Union

from yc_common import config
from yc_common import logging
from yc_common.clients.billing import PublisherAccountCreateRequest
from cloud.marketplace.common.yc_marketplace_common.client.billing import BillingPrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token

log = logging.get_logger("yc_marketplace_queue")


def task(params: TaskParams, *args, **kwargs) -> Union[TaskResolution, None]:
    op = BillingPrivateClient(
        billing_url=config.get_value("endpoints.billing.url"),
        iam_token=metadata_token.get_instance_metadata_token(),
    ).create_publisher_account(PublisherAccountCreateRequest(params.params["billing"]))

    publisher_account_id = op.metadata["publisherAccountId"]

    return TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED, data={
            "operation_id": op.id,
            "publisher_account_id": publisher_account_id,
        },
    )
