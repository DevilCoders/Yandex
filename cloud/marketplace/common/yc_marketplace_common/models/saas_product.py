"""Marketplace saas product model"""
from schematics.transforms import whitelist
from schematics.types import DictType
from schematics.types import FloatType
from schematics.types import IntType
from schematics.types import ListType
from schematics.types import ModelType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.product_slug import ProductSlugResponse
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.misc import timestamp
from yc_common.models import IsoTimestampType
from yc_common.models import JsonDictType
from yc_common.models import JsonListType
from yc_common.models import MetadataOptions
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.models import _PUBLIC_API_ROLE
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class _Status:
    CREATING = "creating"
    PENDING = "pending"
    ACTIVE = "active"
    DEPRECATED = "deprecated"
    REJECTED = "rejected"
    SUSPENDED = "rejected"
    ERROR = "error"
    ALL = {CREATING, PENDING, ACTIVE, DEPRECATED, SUSPENDED, REJECTED, ERROR}
    PUBLIC = {ACTIVE}


class SaasProductCreateRequest(MktBasePublicModel):
    billing_account_id = ResourceIdType(required=True)
    labels = DictType(StringType)
    vendor = StringType()
    name = StringType(required=True)
    description = StringType(required=True)
    short_description = StringType(required=True)
    logo_id = StringType()
    eula_id = StringType()
    category_ids = ListType(ResourceIdType)
    meta = JsonDictType(JsonDictType(StringType), default={})
    slug = StringType()
    sku_ids = ListType(ResourceIdType, default=[])


class SaasProductUpdateRequest(MktBasePublicModel):
    product_id = ResourceIdType(required=True,
                                metadata={MetadataOptions.QUERY_VARIABLE: "product_id"})
    labels = DictType(StringType)
    name = StringType()
    vendor = StringType()
    description = StringType()
    short_description = StringType()
    logo_id = StringType()
    eula_id = StringType()
    category_ids = ListType(ResourceIdType)
    meta = JsonDictType(JsonDictType(StringType))
    slug = StringType()
    sku_ids = ListType(ResourceIdType, default=[])


class SaasProductAddToCategoryRequest(MktBasePublicModel):
    product_id = ResourceIdType(required=True,
                                metadata={MetadataOptions.QUERY_VARIABLE: "product_id"})
    order = DictType(IntType)


class SaasProductPutOnPlaceInCategoryRequest(MktBasePublicModel):
    product_id = ResourceIdType(required=True,
                                metadata={MetadataOptions.QUERY_VARIABLE: "product_id"})
    category_id = ResourceIdType(required=True)
    position = IntType(required=True)


class SaasProductPublishRequest(MktBasePublicModel):
    product_id = ResourceIdType(required=True,
                                metadata={MetadataOptions.QUERY_VARIABLE: "product_id"})


class SaasProduct(AbstractMktBase):
    PublicModel = "SaasProductResponse"
    Status = _Status
    Filterable_fields = {
        "id",
        "created_at",
        "updated_at",
        "name",
        "description",
        "status",
        "billing_account_id",
        "vendor",
    }
    data_model = DataModel((
        Table(name="saas_product", spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                "created_at": KikimrDataType.UINT64,
                "updated_at": KikimrDataType.UINT64,
                "billing_account_id": KikimrDataType.UTF8,
                "vendor": KikimrDataType.UTF8,
                "labels": KikimrDataType.JSON,
                "name": KikimrDataType.UTF8,
                "description": KikimrDataType.UTF8,
                "short_description": KikimrDataType.UTF8,
                "logo_id": KikimrDataType.UTF8,
                "logo_uri": KikimrDataType.UTF8,
                "eula_id": KikimrDataType.UTF8,
                "eula_uri": KikimrDataType.UTF8,
                "status": KikimrDataType.UTF8,
                "meta": KikimrDataType.JSON,
                "score": KikimrDataType.DOUBLE,
                "sku_ids": KikimrDataType.JSON,
            },
            primary_keys=["id"],
        )),
    ))

    class Options:
        roles = {
            _PUBLIC_API_ROLE: whitelist(
                "id",
                "created_at",
                "billing_account_id",
                "labels",
                "name",
                "description",
                "short_description",
                "logo_uri",
                "eula_uri",
                "status",
                "vendor",
                "meta",
                "score",
                "sku_ids"
            ),
        }

    id = ResourceIdType(required=True)
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    billing_account_id = ResourceIdType(required=True)
    labels = JsonDictType(StringType)
    name = StringType(required=True)
    description = StringType(required=True)
    short_description = StringType(required=True)
    logo_id = StringType()
    logo_uri = StringType()
    eula_id = StringType()
    eula_uri = StringType()
    vendor = StringType()
    status = StringEnumType(choices=Status.ALL, required=True)
    score = FloatType(required=True)
    meta = JsonDictType(StringType, default={})
    sku_ids = JsonListType(ResourceIdType, default=[])

    @classmethod
    def field_names(cls):
        return {key for key, _ in cls.fields.items()}

    @classmethod
    def db_fields(cls, table_name=""):
        if table_name:
            table_name += "."
        return ",".join("{}{}".format(table_name, key) for key in cls.field_names())

    @classmethod
    def new(cls,
            billing_account_id,
            labels,
            name,
            description,
            short_description,
            vendor,
            logo_id=None,
            eula_id=None,
            meta={},
            sku_ids=None):
        if not sku_ids:
            sku_ids = []
        return super().new(
            billing_account_id=billing_account_id,
            name=name,
            labels=labels,
            description=description,
            short_description=short_description,
            logo_id=logo_id,
            eula_id=eula_id,
            vendor=vendor,
            created_at=timestamp(),
            updated_at=timestamp(),
            status=cls.Status.CREATING,
            score=0.0,
            meta=meta,
            sku_ids=sku_ids
        )

    @classmethod
    def from_request(cls, request: SaasProductCreateRequest) -> "SaasProduct":
        return cls.new(
            billing_account_id=request.billing_account_id,
            name=request.name,
            labels=request.labels,
            description=request.description,
            short_description=request.short_description,
            vendor=request.vendor or None,
            meta=request.meta or {},
            logo_id=request.logo_id,
            eula_id=request.eula_id,
            sku_ids=request.sku_ids or [],
        )


class SaasProductResponse(MktBasePublicModel):
    id = ResourceIdType(required=True)
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    billing_account_id = ResourceIdType(required=True)
    labels = JsonDictType(StringType)
    name = StringType(required=True)
    vendor = StringType()
    description = StringType(required=True)
    short_description = StringType(required=True)
    logo_id = StringType()
    logo_uri = StringType()
    eula_id = StringType()
    eula_uri = StringType()
    status = StringEnumType(choices=_Status.ALL, required=True)
    score = FloatType(required=True)
    category_ids = ListType(ResourceIdType)
    slugs = ListType(ModelType(ProductSlugResponse))
    meta = JsonDictType(StringType)
    sku_ids = JsonListType(ResourceIdType)
    # internal
    order = IntType()

    class Options:
        roles = {
            _PUBLIC_API_ROLE: whitelist(
                "id",
                "created_at",
                "billing_account_id",
                "labels",
                "name",
                "description",
                "short_description",
                "logo_id",
                "logo_uri",
                "eula_id",
                "eula_uri",
                "status",
                "category_ids",
                "score",
                "vendor",
                "meta",
                "slugs",
                "updated_at",
                "sku_ids",
            ),
        }


class SaasProductList(MktBasePublicModel):
    next_page_token = StringType()
    products = ListType(ModelType(SaasProductResponse), required=True, default=list)
    Options = get_model_options(public_api_fields=("next_page_token", "products"))


class SaasProductMetadata(MktBasePublicModel):
    saas_product_id = ResourceIdType()


class SaasProductOperation(OperationV1Beta1):
    metadata = ModelType(SaasProductMetadata)
    response = ModelType(SaasProductResponse)
