from decimal import Decimal

from yc_common.clients.models import BasePublicModel
from yc_common.fields import DateType
from yc_common.fields import JsonSchemalessDictType
from yc_common.fields import IntType
from yc_common.fields import ListType
from yc_common.fields import ModelType
from yc_common.fields import SchemalessDictType
from yc_common.fields import StringType
from yc_common.models import Model
from yc_common.models import NormalizedDecimalType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class RevenueReportRequest(BasePublicModel):
    start_date = DateType(required=True)
    end_date = DateType(required=True)
    sku_ids = ListType(ResourceIdType, required=False, default=list)


class RevenueMetaSku(BasePublicModel):
    id = ResourceIdType()
    name = StringType()
    pricing_unit = StringType()
    service_id = ResourceIdType()

    Options = get_model_options(public_api_fields=(
        "id",
        "name",
        "pricing_unit",
        "service_id",
    ))


class RevenueMeta(BasePublicModel):
    start_date = DateType()
    end_date = DateType()
    skus = ListType(ModelType(RevenueMetaSku), default=list)

    Options = get_model_options(public_api_fields=(
        "start_date",
        "end_date",
        "skus",
    ))


class RevenueMetaResponse(BasePublicModel):
    revenue_meta = ModelType(RevenueMeta)

    Options = get_model_options(public_api_fields=(
        "revenue_meta",
    ))


class RevenueAggregateReport(BasePublicModel):
    sum_total = NormalizedDecimalType(default=Decimal(0))
    entities_data = JsonSchemalessDictType(default=dict)

    Options = get_model_options(public_api_fields=(
        "sum_total",
        "entities_data",
    ))


class PeriodicRevenue(BasePublicModel):
    id = ResourceIdType()
    period = DateType()
    total = NormalizedDecimalType()

    Options = get_model_options(public_api_fields=(
        "period",
        "total",
    ))


class RevenueReportBaseEntity(BasePublicModel):
    entity_total = NormalizedDecimalType(default=Decimal(0))
    meta = SchemalessDictType()
    periodic = ListType(ModelType(PeriodicRevenue), default=list)

    Options = get_model_options(public_api_fields=(
        "entity_total",
        "meta",
        "periodic",
    ))


class RevenueReport(Model):
    publisher_account_id = ResourceIdType(required=True)
    publisher_balance_client_id = StringType()
    billing_account_id = ResourceIdType(required=True)
    date = StringType(required=True)  # MSK Timezone
    sku_id = ResourceIdType(required=True)
    total = NormalizedDecimalType(required=True)
    created_at = IntType(required=True)
