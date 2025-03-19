from schematics.types import IntType
from schematics.types import ListType
from schematics.types import ModelType
from schematics.types import StringType
from yc_common.clients.models import BasePublicModel
from yc_common.models import NormalizedDecimalType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class _Rate(BasePublicModel):
    start_pricing_quantity = NormalizedDecimalType(required=True,
                                                   metadata={"label": "Tier quantity start in pricing units"})
    unit_price = NormalizedDecimalType(required=True, metadata={"label": "Price in rubles"})

    Options = get_model_options(public_api_fields=(
        "start_pricing_quantity",
        "unit_price",
    ))


class _AggregationInfo(BasePublicModel):
    level = StringType(required=True, choices=["billing_account", "cloud"],
                       metadata={"label": "Aggregation level"})
    interval = StringType(required=True, choices=["month", "date"], default="month",
                          metadata={"label": "Aggregation interval"})

    Options = get_model_options(public_api_fields=(
        "level",
        "interval",
    ))


class _PricingExpression(BasePublicModel):
    quantum = NormalizedDecimalType(required=True, default=0,
                                    metadata={"label": "Min valuable pricing unit number"})
    rates = ListType(ModelType(_Rate), min_size=1)

    Options = get_model_options(public_api_fields=(
        "quantum",
        "rates"
    ))


class PricingVersion(BasePublicModel):

    # pseudo id, so we would be able to edit/delete this entity
    id = ResourceIdType(required=True, metadata={"label": "Pricing version id"})
    effective_time = IntType(required=True, metadata={"label": "Effective time"})
    aggregation_info = ModelType(_AggregationInfo)  # ToDo: required
    pricing_expression = ModelType(_PricingExpression)  # ToDo: required

    Options = get_model_options(public_api_fields=(
        "id",
        "effective_time",
        "aggregation_info",
        "pricing_expression"
    ))
