from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.product_type import ProductType
from yc_common.models import Model
from yc_common.models import StringEnumType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class ProductReference(Model):
    product_type = StringEnumType(required=True, choices={ProductType.SAAS})
    product_id = ResourceIdType(required=True)


class ProductReferencePublicModel(MktBasePublicModel):
    product_type = StringEnumType(required=True, choices={ProductType.SAAS})
    product_id = ResourceIdType(required=True)

    Options = get_model_options(public_api_fields=(
        "product_type",
        "product_id",
    ))
