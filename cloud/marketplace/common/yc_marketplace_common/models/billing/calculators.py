from yc_common.clients.models.base import BasePublicModel
from yc_common.fields import IntType
from yc_common.fields import ListType
from yc_common.fields import ModelType
from yc_common.fields import SchemalessDictType
from yc_common.fields import StringType
from yc_common.models import NormalizedDecimalType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType

DEFAULT_VERSION = "v1alpha1"


class Currencies:
    RUB = "RUB"
    USD = "USD"
    RUR = "RUR"

    ALL = [RUB, RUR, USD]
    SELECTABLE = [RUB, USD]


class UsageType:
    CUMULATIVE = "cumulative"
    DELTA = "delta"
    GAUGE = "gauge"

    ALL = [CUMULATIVE, DELTA, GAUGE]


class UsageRequest(BasePublicModel):
    quantity = IntType(required=False)
    start = IntType(required=True)  # UTC timestamp
    finish = IntType(required=True)  # UTC timestamp
    unit = StringType(required=False)  # bytes | seconds | calls | ...
    type = StringType(required=False, default=UsageType.DELTA, choices=UsageType.ALL)

    Options = get_model_options(public_api_fields=(
        "quantity",
        "start",
        "finish",
        "unit",
        "type",
    ))


class LightMetricRequest(BasePublicModel):
    schema = StringType(required=True)
    usage = ModelType(UsageRequest, required=True)
    tags = SchemalessDictType()
    sku_id = StringType()
    pricing_quantity = NormalizedDecimalType()

    # rogue fields
    id = StringType()
    billing_account_id = StringType()
    cloud_id = StringType()
    folder_id = StringType()
    labels = SchemalessDictType()
    resource_id = StringType()
    source_id = StringType()
    source_wt = IntType()
    version = StringType(default=DEFAULT_VERSION)


class LightMetricResponse(BasePublicModel):
    schema = StringType(required=True)
    usage = ModelType(UsageRequest, required=True)
    tags = SchemalessDictType()
    sku_id = StringType()

    pricing_quantity = NormalizedDecimalType()
    # rogue fields
    id = StringType()
    billing_account_id = StringType()
    cloud_id = StringType()
    folder_id = StringType()
    labels = SchemalessDictType()
    resource_id = StringType()
    source_id = StringType()
    source_wt = IntType()
    version = StringType(default=DEFAULT_VERSION)

    Options = get_model_options(public_api_fields=(
        "schema",
        "usage",
        "tags",
        "sku_id",
        "id",
        "billing_account_id",
        "cloud_id",
        "folder_id",
        "labels",
        "resource_id",
        "source_id",
        "source_wt",
        "version",
        "pricing_quantity"
    ))


class DryRunRequest(BasePublicModel):
    cloud_id = ResourceIdType()
    currency = StringType(choices=Currencies.SELECTABLE)
    metrics = ListType(ModelType(LightMetricRequest), required=True)


class CalculatedItem(BasePublicModel):
    sku_id = ResourceIdType(required=True)
    pricing_quantity = NormalizedDecimalType(required=True)
    cost = NormalizedDecimalType(required=True)

    Options = get_model_options(public_api_fields=("sku_id", "pricing_quantity", "cost"))


class DryRunResponse(BasePublicModel):
    currency = StringType(required=True)
    calculated_items = ListType(ModelType(CalculatedItem), required=True)
    unresolved_metrics = ListType(ModelType(LightMetricResponse), required=False, default=[])

    Options = get_model_options(public_api_fields=(
        "currency",
        "calculated_items",
        "unresolved_metrics"))
