from typing import Union

from yc_common import logging
from cloud.marketplace.common.yc_marketplace_common.lib import Isv
from cloud.marketplace.common.yc_marketplace_common.lib import Publisher
from cloud.marketplace.common.yc_marketplace_common.lib import Var
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerTypes
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution

log = logging.get_logger("yc_marketplace_queue")


def task(depends, params, *args, **kwargs) -> Union[TaskResolution, None]:
    diff = params["params"]["diff"]
    for depend in depends:
        if "url" in depend.response:
            diff["logo_uri"] = depend.response["url"]

    if params["params"]["type"] == PartnerTypes.ISV:
        Isv.update(params["params"]["id"], diff)
    elif params["params"]["type"] == PartnerTypes.VAR:
        Var.update(params["params"]["id"], diff)
    elif params["params"]["type"] == PartnerTypes.PUBLISHER:
        Publisher.update(params["params"]["id"], diff)
    else:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED, data={
                "message": "Partner type is not implemented",
            },
        )

    return TaskResolution.resolve(status=TaskResolution.Status.RESOLVED, data={})
