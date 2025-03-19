from typing import Type

from schematics.types import ListType
from schematics.types import ModelType
from schematics.types import StringType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.billing.pricing import PricingVersion
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.clients.models import BasePublicModel
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.fields import DictType
from yc_common.fields import JsonDictType
from yc_common.fields import JsonListType
from yc_common.misc import timestamp
from yc_common.models import IsoTimestampType
from yc_common.models import Model
from yc_common.models import NormalizedDecimalType
from yc_common.models import StringEnumType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType
from yc_common.validation import ResourceNameType


class SkuDraftStatus:
    PENDING = "pending"
    ACTIVE = "active"
    REJECTED = "rejected"
    ALL = {ACTIVE, REJECTED, PENDING}
    PUBLIC = {ACTIVE}


class SkuDraftUnit:
    CORE_PER_SECOND = "core-per-second"
    INSTANCE_PER_SECOND = "instance-per-second"

    ALL = {CORE_PER_SECOND, INSTANCE_PER_SECOND}
    # CLOUD-48682: x/1.2*0.8
    # consts MUST BE in single quotes. It is the only correct way to init Decimal
    DEFAULT_RATE_FORMULA = "mul(div(add(cost, credit), '1.2'), '0.8')"
    __MAP = {
        CORE_PER_SECOND: {
            "formula": "mul(tags.cores, usage.quantity)",
            "metric_unit": "second",
            "usage_unit": "core*second",
            "pricing_unit": "core*hour",
            "resolving_policy": "`false`",
            "rate_formula": DEFAULT_RATE_FORMULA,
            "schemas": ["compute.vm.generic.v1"],
        },
        INSTANCE_PER_SECOND: {
            "formula": "usage.quantity",
            "metric_unit": "second",
            "usage_unit": "second",
            "pricing_unit": "hour",
            "resolving_policy": "`false`",
            "rate_formula": DEFAULT_RATE_FORMULA,
            "schemas": ["compute.vm.generic.v1"],
        }
    }

    @classmethod
    def formula(cls, val):
        return cls.__MAP[val]["formula"]

    @classmethod
    def metric_unit(cls, val):
        return cls.__MAP[val]["metric_unit"]

    @classmethod
    def usage_unit(cls, val):
        return cls.__MAP[val]["usage_unit"]

    @classmethod
    def pricing_unit(cls, val):
        return cls.__MAP[val]["pricing_unit"]

    @classmethod
    def resolving_policy(cls, val):
        return cls.__MAP[val]["resolving_policy"]

    @classmethod
    def rate_formula(cls, val):
        return cls.__MAP[val]["rate_formula"]

    @classmethod
    def schemas(cls, val):
        return cls.__MAP[val]["schemas"]


class AggregationInfo(Model):
    class AggregationLevels:
        BILLING_ACCOUNT = "billing_account"

        ALL = [BILLING_ACCOUNT]

    class AggregationIntervals:
        MONTH = "month"

        ALL = [MONTH]

    level = StringType(required=True, choices=AggregationLevels.ALL, default=AggregationLevels.BILLING_ACCOUNT)
    interval = StringType(required=True, choices=AggregationIntervals.ALL, default=AggregationIntervals.MONTH)


class AggregationInfoRequest(MktBasePublicModel):
    level = StringType(required=True, choices=AggregationInfo.AggregationLevels.ALL)
    interval = StringType(required=True, choices=AggregationInfo.AggregationIntervals.ALL)


class RateRequest(MktBasePublicModel):
    start_pricing_quantity = NormalizedDecimalType(required=True)
    unit_price = NormalizedDecimalType(required=True)


class PricingExpressionRequest(MktBasePublicModel):
    quantum = NormalizedDecimalType(required=True, default=0)
    rates = ListType(ModelType(RateRequest), required=True, min_size=1)

    def to_model(self):
        rates = [Rate.from_api(rate) for rate in self.rates]

        return PricingExpression.new(rates=rates, quantum=self.quantum)


class PricingVersionRequest(MktBasePublicModel):
    aggregation_info = ModelType(AggregationInfoRequest)
    pricing_expression = ModelType(PricingExpressionRequest, required=True)

    def to_model(self):
        if self.aggregation_info:
            aggregation_info = AggregationInfo.from_api(self.aggregation_info)
        else:
            aggregation_info = None

        return PricingVersionDraft.new(
            aggregation_info=aggregation_info,
            pricing_expression=self.pricing_expression.to_model())


class CreateSkuDraftRequest(MktBasePublicModel):
    name = StringType(required=True)
    description = StringType(required=False)
    unit = StringEnumType(required=True, choices=SkuDraftUnit.ALL)
    billing_account_id = ResourceIdType(required=True)
    publisher_account_id = ResourceIdType(required=True)

    pricing_versions = JsonListType(ModelType(PricingVersionRequest), required=True, min_size=1)
    meta = JsonDictType(DictType(StringType), default={})

    def to_model(self):
        return SkuDraft.new(
            id=generate_id(),
            name=self.name,
            description=self.description,
            unit=self.unit,
            billing_account_id=self.billing_account_id,
            publisher_account_id=self.publisher_account_id,
            pricing_versions=[pv.to_model() for pv in self.pricing_versions],
            created_at=timestamp(),
            updated_at=timestamp(),
            status=SkuDraftStatus.PENDING,
            meta=self.meta,
        )


class Rate(Model):
    start_pricing_quantity = NormalizedDecimalType(required=True)
    unit_price = NormalizedDecimalType(required=True)


class PricingExpression(Model):
    quantum = NormalizedDecimalType(required=True, default=0)
    rates = ListType(ModelType(Rate), required=True, min_size=1)


class PricingVersionDraft(Model):
    aggregation_info = ModelType(AggregationInfo)
    pricing_expression = ModelType(PricingExpression, required=True)

    @property
    def tiered(self):
        return self.get("aggregation_info") is not None


class SkuDraft(AbstractMktBase):
    @property
    def PublicModel(self) -> Type[BasePublicModel]:
        return SkuDraftResponse

    id = ResourceIdType(default=generate_id)
    billing_account_id = ResourceIdType(required=True)
    name = ResourceNameType(required=True)
    description = StringType(required=True)
    unit = StringType(required=True, choices=SkuDraftUnit.ALL)  # for resolve_skus
    publisher_account_id = ResourceIdType(required=True)
    pricing_versions = JsonListType(ModelType(PricingVersionDraft), required=True, min_size=1)
    status = StringType(required=True, choices=SkuDraftStatus.ALL)
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType(required=True)
    meta = JsonDictType(DictType(StringType), default={})

    data_model = DataModel((
        Table(name="sku_draft", spec=KikimrTableSpec(
            columns={
                "billing_account_id": KikimrDataType.UTF8,
                "id": KikimrDataType.UTF8,
                "status": KikimrDataType.UTF8,
                "publisher_account_id": KikimrDataType.UTF8,
                "name": KikimrDataType.UTF8,
                "description": KikimrDataType.UTF8,
                "pricing_versions": KikimrDataType.JSON,
                "unit": KikimrDataType.UTF8,
                "created_at": KikimrDataType.UINT64,
                "updated_at": KikimrDataType.UINT64,
                "meta": KikimrDataType.JSON,
            },
            primary_keys=["billing_account_id", "id"],
        )),
    ))

    Filterable_fields = {
        "id",
    }

    @property
    def formula(self):
        return SkuDraftUnit.formula(self.unit)

    @property
    def metric_unit(self):
        return SkuDraftUnit.metric_unit(self.unit)

    @property
    def usage_unit(self):
        return SkuDraftUnit.usage_unit(self.unit)

    @property
    def pricing_unit(self):
        return SkuDraftUnit.pricing_unit(self.unit)

    @property
    def resolving_policy(self):
        return SkuDraftUnit.resolving_policy(self.unit)

    @property
    def rate_formula(self):
        return SkuDraftUnit.rate_formula(self.unit)

    @property
    def schemas(self):
        return SkuDraftUnit.schemas(self.unit)

    def to_sku_pricing_versions(self, effective_time):
        return [self.__make_billing_pricing_version(pv, effective_time) for pv in self.pricing_versions]

    def __make_billing_pricing_version(self, pvd: PricingVersionDraft, effective_time):
        aggregation_info = pvd.aggregation_info.to_primitive() if pvd.aggregation_info else None
        pricing_expression = pvd.pricing_expression.to_primitive() if pvd.pricing_expression else None
        return PricingVersion({
            "id": generate_id(),
            "effective_time": effective_time,
            "aggregation_info": aggregation_info,
            "pricing_expression": pricing_expression,
        })


class AggregationInfoResponse(MktBasePublicModel):
    level = StringType(required=True)
    interval = StringType(required=True)

    Options = get_model_options(public_api_fields=(
        "level",
        "interval",
    ))


class RateResponse(MktBasePublicModel):
    start_pricing_quantity = NormalizedDecimalType(required=True)
    unit_price = NormalizedDecimalType(required=True)

    Options = get_model_options(public_api_fields=(
        "start_pricing_quantity",
        "unit_price",
    ))


class PricingExpressionResponse(MktBasePublicModel):
    quantum = NormalizedDecimalType(required=True, default=0)
    rates = ListType(ModelType(RateResponse), required=True, min_size=1)

    Options = get_model_options(public_api_fields=(
        "quantum",
        "rates",
    ))


class PricingVersionResponse(MktBasePublicModel):
    aggregation_info = ModelType(AggregationInfoResponse)
    pricing_expression = ModelType(PricingExpressionResponse, required=True)

    @property
    def tiered(self):
        return self.get("aggregation_info") is not None

    Options = get_model_options(public_api_fields=(
        "aggregation_info",
        "pricing_expression",
    ))


class SkuDraftResponse(MktBasePublicModel):
    id = ResourceIdType()
    status = StringEnumType(required=True, choices=SkuDraftStatus.ALL)
    name = StringType(required=True)
    description = StringType(required=False)
    unit = StringEnumType(required=True)
    publisher_account_id = ResourceIdType()
    billing_account_id = ResourceIdType()
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    meta = JsonDictType(DictType(StringType), default={})
    pricing_versions = ListType(ModelType(PricingVersionResponse), required=True, min_size=1)

    Options = get_model_options(public_api_fields=(
        "id",
        "status",
        "name",
        "unit",
        "publisher_account_id",
        "created_at",
        "updated_at",
        "pricing_versions",
        "meta",
    ))


class SkuDraftMetadata(MktBasePublicModel):
    sku_draft_id = ResourceIdType(required=True)


class SkuDraftOperation(OperationV1Beta1):
    metadata = ModelType(SkuDraftMetadata, required=True)
    response = ModelType(SkuDraftResponse)


class SkuDraftList(MktBasePublicModel):
    next_page_token = StringType()
    sku_drafts = ListType(ModelType(SkuDraftResponse), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "sku_drafts"))


class SkuDraftListingRequest(BaseMktManageListingRequest):
    pass
