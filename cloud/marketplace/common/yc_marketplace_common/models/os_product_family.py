"""Marketplace product family version model"""
from typing import Set

from schematics.types import DictType
from schematics.types import ListType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.deprecation import Deprecation
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersion as OsProductFamilyVersionScheme
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import RuleSpec
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import SkuPublicModel
from cloud.marketplace.common.yc_marketplace_common.models.product_reference import ProductReferencePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.resource_spec import ResourceSpecPublicModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidStatus
from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.clients.models.base import BasePublicListingRequest
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.models import IsoTimestampType
from yc_common.models import JsonDictType
from yc_common.models import JsonListType
from yc_common.models import JsonModelType
from yc_common.models import JsonSchemalessDictType
from yc_common.models import MetadataOptions
from yc_common.models import ModelType
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType
from yc_common.validation import ResourceNameType


class _PricingOptions:
    FREE = "free"
    BYOL = "byol"
    PAYG = "payg"
    RESERVE = "reserve"
    TRIAL = "trial"
    MONTHLY = "monthly"
    ALL = {FREE, BYOL, PAYG, RESERVE, TRIAL, MONTHLY}


_V_Status = OsProductFamilyVersionScheme.Status


class _Status:
    NEW = "new"
    PENDING = "pending"
    ACTIVE = "active"
    DEPRECATED = "deprecated"
    REJECTED = "rejected"
    ERROR = "error"

    ALL = {NEW, PENDING, ACTIVE, DEPRECATED, REJECTED, ERROR}
    PUBLIC = {ACTIVE}

    @staticmethod
    def from_version(ver):
        return {
            _V_Status.ACTIVE: _Status.ACTIVE,
            _V_Status.ACTIVATING: _Status.PENDING,
            _V_Status.DEPRECATED: _Status.DEPRECATED,
            _V_Status.PENDING: _Status.PENDING,
            _V_Status.ERROR: _Status.ERROR,
            _V_Status.REJECTED: _Status.REJECTED,
            _V_Status.REVIEW: _Status.PENDING,
        }[ver.status]


class OsProductFamilyCreateRequest(MktBasePublicModel):
    billing_account_id = ResourceIdType(required=True)
    labels = JsonSchemalessDictType()

    name = StringType(required=True)
    description = StringType(default="")
    image_id = ResourceIdType(required=True)
    pricing_options = StringEnumType(choices=_PricingOptions.ALL)
    resource_spec = ModelType(ResourceSpecPublicModel, required=True)
    os_product_id = ResourceIdType(required=True)
    logo_id = StringType()
    skus = JsonListType(ModelType(SkuPublicModel))
    slug = ResourceNameType()
    meta = JsonDictType(JsonDictType(StringType))
    related_products = JsonListType(ModelType(ProductReferencePublicModel))
    license_rules = JsonListType(ModelType(RuleSpec))


class OsProductFamilyUpdateRequest(MktBasePublicModel):
    os_product_family_id = ResourceIdType(required=True,
                                          metadata={MetadataOptions.QUERY_VARIABLE: "os_product_family_id"})
    update_mask = DictType(StringType)  # TODO
    labels = DictType(StringType)
    name = StringType()
    description = StringType()

    # Version fields
    image_id = ResourceIdType()
    resource_spec = ModelType(ResourceSpecPublicModel)
    pricing_options = StringEnumType(choices=_PricingOptions.ALL)
    logo_id = StringType()
    skus = JsonListType(ModelType(SkuPublicModel))
    slug = ResourceNameType()
    meta = JsonDictType(JsonDictType(StringType))
    related_products = JsonListType(ModelType(ProductReferencePublicModel))
    license_rules = JsonListType(ModelType(RuleSpec))


class OsProductFamilyDeprecationRequest(MktBasePublicModel):
    os_product_family_id = ResourceIdType(required=True,
                                          metadata={MetadataOptions.QUERY_VARIABLE: "os_product_family_id"})
    deprecation = ModelType(Deprecation)


class OsProductFamily(AbstractMktBase):
    @property
    def PublicModel(self):
        return OsProductFamilyResponse

    Status = _Status
    PricingOptions = _PricingOptions

    data_model = DataModel((
        Table(name="os_product_family", spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                "billing_account_id": KikimrDataType.UTF8,
                "created_at": KikimrDataType.UINT64,
                "updated_at": KikimrDataType.UINT64,
                "labels": KikimrDataType.JSON,
                "name": KikimrDataType.UTF8,
                "description": KikimrDataType.UTF8,
                "status": KikimrDataType.UTF8,
                "os_product_id": KikimrDataType.UTF8,
                "deprecation": KikimrDataType.JSON,
                "meta": KikimrDataType.JSON,
            },
            primary_keys=["os_product_id", "id"],
        )),
    ))

    id = ResourceIdType(required=True)
    billing_account_id = ResourceIdType(required=True)
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    labels = JsonDictType(StringType)
    name = StringType(required=True)
    description = StringType()
    status = StringEnumType(choices=_Status.ALL, required=True)
    os_product_id = ResourceIdType(required=True)
    deprecation = JsonModelType(Deprecation)
    meta = JsonDictType(StringType)

    Filterable_fields = {
        "id",
        "created_at",
        "updated_at",
        "name",
        "description",
        "status",
        "os_product_id",
    }

    @classmethod
    def field_names(cls) -> Set[str]:
        return {key for key, _ in cls.fields.items()}

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."

        return ",".join("{}{}".format(table_name, key) for key in cls.field_names())

    @classmethod
    def new(cls,
            billing_account_id,
            os_product_id,
            name,
            description,
            status=None,
            id=None,
            created_at=None,
            meta=None) -> "OsProductFamily":
        status_new = cls.Status.NEW

        if status is not None and status not in cls.Status.ALL:
            raise InvalidStatus()

        return super().new(
            billing_account_id=billing_account_id,
            name=name,
            description=description,
            updated_at=None,
            os_product_id=os_product_id,
            meta=meta,
            status=status_new if status is None else status,
        )


class OsProductFamilyResponse(MktBasePublicModel):
    id = ResourceIdType(required=True)
    billing_account_id = ResourceIdType(required=True)
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    labels = JsonDictType(StringType)
    name = StringType(required=True)
    description = StringType()
    status = StringEnumType(choices=_Status.ALL, required=True)
    os_product_id = ResourceIdType(required=True)
    deprecation = JsonModelType(Deprecation)
    active_version = ModelType(OsProductFamilyVersionResponse)
    version = ModelType(OsProductFamilyVersionResponse)
    meta = JsonDictType(StringType, default={})

    Options = get_model_options(public_api_fields=(
        "id",
        "billing_account_id",
        "created_at",
        "updated_at",
        "labels",
        "name",
        "description",
        "status",
        "os_product_id",
        "deprecation",
        "meta",
        "active_version",
        "version",
    ))


class OsProductFamilyList(MktBasePublicModel):
    next_page_token = StringType()
    os_product_families = ListType(ModelType(OsProductFamilyResponse), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "os_product_families"))


class OsProductFamilyMetadata(MktBasePublicModel):
    os_product_family_id = ResourceIdType()


class OsProductFamilyOperation(OperationV1Beta1):
    metadata = ModelType(OsProductFamilyMetadata)
    response = ModelType(OsProductFamilyResponse)


class OsProductFamilySearchRequest(BaseMktManageListingRequest):
    order_by = StringType(choices=OsProductFamily.field_names())
    filter = StringType()


class OsProductFamilyListingRequest(BasePublicListingRequest):
    order_by = StringType(choices=OsProductFamily.field_names())
    filter = StringType()
