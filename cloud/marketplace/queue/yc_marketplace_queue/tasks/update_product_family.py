from cloud.marketplace.common.yc_marketplace_common.lib import OsProductFamily
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution


def task(id: str, params: TaskParams, **kwargs) -> TaskResolution:
    OsProductFamily.update(params.params["os_product_family_id"], params.params["update"])

    return TaskResolution.resolve(status=TaskResolution.Status.RESOLVED)  # TODO fill data
