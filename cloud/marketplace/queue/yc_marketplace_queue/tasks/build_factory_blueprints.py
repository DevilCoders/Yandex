from typing import Union

from yc_common import config
from yc_common import logging
from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.lib import Blueprint
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintStatus
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token
from cloud.marketplace.queue.yc_marketplace_queue.exceptions import TaskParamValidationError
from cloud.marketplace.queue.yc_marketplace_queue.models.params import BuildBlueprintsParams
from cloud.marketplace.queue.yc_marketplace_queue.utils.checks import get_params

log = logging.get_logger("yc_marketplace_queue")


def task(params: dict, *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        build_blueprints_params = get_params(params["params"], BuildBlueprintsParams)
    except TaskParamValidationError as e:
        return e.resolution_resolved
    marketplace_client = MarketplacePrivateClient(config.get_value("endpoints.marketplace_private.url"),
                                                  iam_token=metadata_token.get_instance_metadata_token(),
                                                  timeout=20)

    filter = "publisher_account_id='{}' and status='{}'".format(build_blueprints_params.publisher_account_id,
                                                                BlueprintStatus.ACTIVE)
    list = marketplace_client.list_blueprints(filter_query=filter)

    for blueprint in list.blueprints:
        Blueprint.rpc_build(blueprint.id)

    log.debug("Created %s tasks for blueprints.", len(list))

    return TaskResolution.resolve(status=TaskResolution.Status.RESOLVED)
