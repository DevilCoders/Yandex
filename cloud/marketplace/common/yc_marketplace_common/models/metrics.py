from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.billing.calculators import LightMetricResponse
from cloud.marketplace.common.yc_marketplace_common.models.product_reference import ProductReferencePublicModel
from yc_common import fields
from yc_common.models import get_model_options


class SCHEMAS:
    COMPUTE_GENERIC = "compute.vm.generic.v1"
    MKT_SAAS = "marketplace.saas.product"
    MKT_IMAGE = "marketplace.image.product"


class ServiceMetricsResponse(MktBasePublicModel):
    metrics = fields.ListType(fields.ModelType(LightMetricResponse), required=True)
    links = fields.DictType(fields.ModelType(ProductReferencePublicModel), required=False)

    Options = get_model_options(public_api_fields=(
        "metrics",
        "links",
    ))
