from schematics import Model
from schematics.types import IntType
from schematics.types import ListType
from schematics.types import ModelType
from schematics.types import StringType
from yc_common.clients.models import BasePublicModel
from yc_common.clients.models.billing.pricing import PricingVersion
from yc_common.fields import DictType
from yc_common.fields import IntType
from yc_common.fields import ListType
from yc_common.fields import ModelType
from yc_common.fields import StringType
from yc_common.fields import BooleanType
from yc_common.models import JsonListType
from yc_common.models import Model
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class SkuProductLinkRequest(Model):
    sku_id = ResourceIdType(required=True)
    product_id = ResourceIdType(required=True)
    check_formula = StringType(required=True)


class CreateSkuRequest(BasePublicModel):
    name = StringType(required=True, metadata={"label": "SKU name"})
    service_id = ResourceIdType(required=True, metadata={"label": "Service id"})
    balance_product_id = StringType(required=True,
                                    metadata={"label": "SKU Balance ID"})
    metric_unit = StringType(required=True, metadata={"label": "Unit from metric"})  # for resolve_skus
    usage_unit = StringType(required=True, metadata={"label": "Unit for sku resolver"})  # for resolve_skus
    pricing_unit = StringType(required=True, metadata={"label": "Unit for pricing"})  # for build_sku_counter
    pricing_versions = JsonListType(ModelType(PricingVersion), min_size=1)
    formula = StringType()  # DSL for computing quantity
    publisher_account_id = ResourceIdType()
    rate_formula = StringType()  # DSL for computing quantity
    resolving_policy = StringType()  # DSL for resolving policy description

    schemas = ListType(StringType)
    is_unique_schema = BooleanType()
    translations = DictType(DictType(StringType()), key=StringType(), default=dict)


class SkuPublicView(BasePublicModel):
    id = ResourceIdType(metadata={"label": "SKU ID"})
    name = StringType(required=True, metadata={"label": "SKU name"})
    service_id = ResourceIdType(required=True, metadata={"label": "Service id"})
    metric_unit = StringType(required=True, metadata={"label": "Unit from metric"})  # for resolve_skus
    usage_unit = StringType(required=True, metadata={"label": "Unit for sku resolver"})  # for resolve_skus
    pricing_unit = StringType(required=True, metadata={"label": "Unit for pricing"})  # for build_sku_counter
    balance_product_id = StringType(required=True,
                                    metadata={"label": "SKU Balance ID"})
    pricing_versions = JsonListType(ModelType(PricingVersion), min_size=1)
    formula = StringType()  # DSL for computing quantity
    resolving_policy = StringType()  # DSL for resolving policy description
    publisher_account_id = ResourceIdType()
    rate_formula = StringType()
    priority = IntType(required=True, default=0)
    created_at = IntType(required=True, metadata={"label": "SKU creation ts"})

    Options = get_model_options(public_api_fields=(
        "id",
        "name",
        "service_id",
        "created_at"
    ))


class SkuList(BasePublicModel):
    next_page_token = StringType()
    skus = ListType(ModelType(SkuPublicView), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "skus"))


class SkuLinkView(BasePublicModel):
    sku_id = ResourceIdType(required=True)
    product_id = ResourceIdType(required=True)
    check_formula = StringType(required=True)
