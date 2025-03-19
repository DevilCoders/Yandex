"""Marketplace product version model"""
from schematics.types import IntType
from schematics.types import ListType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.product_reference import ProductReference
from cloud.marketplace.common.yc_marketplace_common.models.product_reference import ProductReferencePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.resource_spec import ResourceSpec
from cloud.marketplace.common.yc_marketplace_common.models.resource_spec import ResourceSpecPublicModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidStatus
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.clients.models.base import BasePublicListingRequest
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.misc import timestamp
from yc_common.models import IsoTimestampType
from yc_common.models import JsonListType
from yc_common.models import JsonModelType
from yc_common.models import MetadataOptions
from yc_common.models import Model
from yc_common.models import ModelType
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class _PricingOptions:
    FREE = "free"
    BYOL = "byol"
    PAYG = "payg"
    RESERVE = "reserve"
    TRIAL = "trial"
    MONTHLY = "monthly"
    ALL = {FREE, BYOL, PAYG, RESERVE, TRIAL, MONTHLY}


class Sku(Model):
    id = ResourceIdType(required=True)
    check_formula = StringType(required=True)


class SkuPublicModel(MktBasePublicModel):
    id = ResourceIdType(required=True)
    check_formula = StringType(required=True)

    Options = get_model_options(public_api_fields=(
        "id",
        "check_formula",
    ))


class OsProductFamilyVersion(AbstractMktBase):
    @property
    def PublicModel(self):
        return OsProductFamilyVersionResponse

    class Status:
        PENDING = "pending"
        REVIEW = "review"
        ACTIVE = "active"
        ACTIVATING = "activating"
        DEPRECATED = "deprecated"
        REJECTED = "rejected"
        ERROR = "error"

        ALL = {PENDING, REVIEW, ACTIVE, ACTIVATING, DEPRECATED, REJECTED, ERROR}
        PUBLIC = {ACTIVE, DEPRECATED}
        FINAL = {ERROR, REJECTED, DEPRECATED}

    PricingOptions = _PricingOptions

    data_model = DataModel((
        Table(name="os_product_family_version", spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                "created_at": KikimrDataType.UINT64,
                "updated_at": KikimrDataType.UINT64,
                "published_at": KikimrDataType.UINT64,
                "billing_account_id": KikimrDataType.UTF8,
                "image_id": KikimrDataType.UTF8,
                "resource_spec": KikimrDataType.JSON,
                "status": KikimrDataType.UTF8,
                "pricing_options": KikimrDataType.UTF8,
                "os_product_family_id": KikimrDataType.UTF8,
                "logo_id": KikimrDataType.UTF8,
                "logo_uri": KikimrDataType.UTF8,
                "skus": KikimrDataType.JSON,
                "form_id": KikimrDataType.UTF8,
                "related_products": KikimrDataType.JSON,
            },
            primary_keys=["os_product_family_id", "id"],
        )),
    ))

    id = ResourceIdType(required=True)
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    published_at = IsoTimestampType()
    billing_account_id = ResourceIdType(required=True)
    image_id = ResourceIdType()
    logo_id = StringType()
    logo_uri = StringType()
    status = StringEnumType(choices=Status.ALL, required=True)
    resource_spec = JsonModelType(ResourceSpec, required=True)
    pricing_options = StringEnumType(choices=PricingOptions.ALL, required=True)
    os_product_family_id = ResourceIdType(required=True)
    skus = JsonListType(ModelType(Sku), default=list)
    form_id = ResourceIdType()  # TODO required
    related_products = JsonListType(ModelType(ProductReference))

    Filterable_fields = {
        "id",
        "created_at",
        "published_at",
        "image_id",
        "status",
        "os_product_family_id",
    }

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."

        return ",".join("{}{}".format(table_name, key) for key, _ in cls.fields.items())

    @classmethod
    def new(cls,
            billing_account_id,
            os_product_family_id,
            pricing_options,
            image_id=None,
            logo_id=None,
            status=None,
            resource_spec=None,
            skus=None,
            related_products=None,
            ) -> "OsProductFamilyVersion":
        status_new = cls.Status.PENDING

        if status is not None and status not in cls.Status.ALL:
            raise InvalidStatus()

        if skus is None:
            skus = []

        if related_products is None:
            related_products = []

        return super().new(
            id=generate_id(),
            created_at=timestamp(),
            os_product_family_id=os_product_family_id,
            published_at=None,
            billing_account_id=billing_account_id,
            image_id=image_id,
            resource_spec=resource_spec,
            status=status_new if status is None else status,
            pricing_options=pricing_options,
            logo_id=logo_id,
            skus=skus,
            related_products=related_products
        )


class RuleCategory:
    BLACKLIST = "blacklist"
    WHITELIST = "whitelist"

    ALL = [BLACKLIST, WHITELIST]


class RuleEntities:
    BILLING_ACCOUNT = "billing_account"

    ALL = [BILLING_ACCOUNT]


class RuleSpec(MktBasePublicModel):
    """Represents a product licensing rule spec"""

    category = StringType(required=True, choices=RuleCategory.ALL)
    entity = StringType(required=True, choices=RuleEntities.ALL)
    path = StringType(required=True)
    expected = ListType(StringType, required=True)


class OsProductFamilyVersionResponse(MktBasePublicModel):
    id = ResourceIdType(required=True)
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    published_at = IsoTimestampType()
    billing_account_id = ResourceIdType(required=True)
    image_id = ResourceIdType()
    logo_id = StringType()
    logo_uri = StringType()
    status = StringEnumType(choices=OsProductFamilyVersion.Status.ALL, required=True)
    resource_spec = JsonModelType(ResourceSpecPublicModel, required=True)
    pricing_options = StringEnumType(choices=OsProductFamilyVersion.PricingOptions.ALL, required=True)
    os_product_family_id = ResourceIdType(required=True)
    skus = JsonListType(ModelType(SkuPublicModel))
    form_id = ResourceIdType()
    related_products = JsonListType(ModelType(ProductReferencePublicModel))

    Options = get_model_options(public_api_fields=(
        "id",
        "created_at",
        "updated_at",
        "published_at",
        "billing_account_id",
        "image_id",
        "resource_spec",
        "status",
        "pricing_options",
        "os_product_family_id",
        "logo_id",
        "logo_uri",
        "skus",
        "form_id",
        "related_products",
    ))


class OsProductFamilyVersionList(MktBasePublicModel):
    next_page_token = StringType()
    os_product_family_versions = ListType(ModelType(OsProductFamilyVersionResponse), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "os_product_family_versions"))


class OsProductFamilyVersionMetadata(MktBasePublicModel):
    os_product_family_version_id = ResourceIdType()
    os_product_family_id = ResourceIdType()


class OsProductFamilyVersionOperation(OperationV1Beta1):
    metadata = ModelType(OsProductFamilyVersionMetadata)
    response = ModelType(OsProductFamilyVersionResponse)


class BaseOsProductFamilyVersionRequest(MktBasePublicModel):
    os_product_family_version_id = ResourceIdType(required=True,
                                                  metadata={
                                                      MetadataOptions.QUERY_VARIABLE: "os_product_family_version_id",
                                                  })


class OsProductFamilyVersionSetStatusRequest(BaseOsProductFamilyVersionRequest):
    status = StringEnumType(choices=OsProductFamilyVersion.Status.ALL, required=True)


class OsProductFamilyVersionSetImageIdRequest(BaseOsProductFamilyVersionRequest):
    image_id = ResourceIdType(required=True)


class PublishRequest(BaseOsProductFamilyVersionRequest):
    pool_size = IntType(required=True)


class OsProductFamilyVersionLogoIdsRequest(MktBasePublicModel):
    ids = StringType()


class OsProductFamilyVersionBatchIdsRequest(MktBasePublicModel):
    ids = StringType()


class OsProductFamilyVersionListRequest(BasePublicListingRequest):
    filter = StringType()
    order_by = StringType()
    billing_account_id = StringType(required=True)


class OsProductFamilyVersionPrivateListRequest(BasePublicListingRequest):
    filter = StringType()
    order_by = StringType()
