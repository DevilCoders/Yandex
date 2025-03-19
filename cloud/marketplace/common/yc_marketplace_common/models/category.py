from schematics.types import FloatType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from yc_common.clients.kikimr.client import KikimrDataType
from yc_common.clients.kikimr.client import KikimrTableSpec
from yc_common.clients.models.base import BasePublicListingRequest
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.models import IsoTimestampType
from yc_common.models import JsonDictType
from yc_common.models import ListType
from yc_common.models import MetadataOptions
from yc_common.models import Model
from yc_common.models import ModelType
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class _Type:
    PUBLIC = "public"
    CUSTOM = "custom"
    SYSTEM = "system"
    PUBLISHER = "publisher"
    VAR = "var"
    ISV = "isv"

    ALL = {PUBLIC, CUSTOM, SYSTEM, PUBLISHER, VAR, ISV}


class CategoryResponse(MktBasePublicModel):
    id = ResourceIdType(required=True)
    name = StringType(required=True)
    type = StringEnumType(choices=_Type.ALL, required=True)
    score = FloatType(required=True, default=0.0)
    parent_id = ResourceIdType()
    created_at = IsoTimestampType()
    updated_at = IsoTimestampType()

    Options = get_model_options(public_api_fields=(
        "id",
        "name",
        "type",
        "score",
        "parent_id",
        "created_at",
        "updated_at",
    ))


class Category(AbstractMktBase):
    Type = _Type

    id = ResourceIdType(required=True)
    name = StringType(required=True)
    type = StringEnumType(choices=Type.ALL, required=True)
    score = FloatType(required=True, default=0.0)
    parent_id = ResourceIdType()
    created_at = IsoTimestampType()
    updated_at = IsoTimestampType()

    data_model = DataModel((
        Table(name="category", spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                "name": KikimrDataType.UTF8,
                "type": KikimrDataType.UTF8,
                "score": KikimrDataType.DOUBLE,
                "parent_id": KikimrDataType.UTF8,
                "created_at": KikimrDataType.UINT64,
                "updated_at": KikimrDataType.UINT64,
            },
            primary_keys=["id"],
        )),
    ))

    Filterable_fields = {
        "id",
        "name",
        "type",
        "score",
        "parent_id",
        "created_at",
        "updated_at",
    }

    @classmethod
    def new(cls, name, type, score, parent_id) -> "Category":  # noqa
        return super().new(
            name=name,
            type=type,
            score=score,
            parent_id=parent_id,
        )

    @classmethod
    def from_request(cls, request: "CategoryCreateRequest"):
        return cls.new(
            name="",  # request.name is dict for localization
            type=request.type,
            score=request.score,
            parent_id=request.parent_id,
        )

    PublicModel = CategoryResponse

    def to_short(self) -> "CategoryShortView":
        # strict=False to drop unused fields
        return CategoryShortView(self.to_primitive(), strict=False)


class CategoryShortView(Model):
    id = ResourceIdType(required=True)
    name = StringType(required=True)
    parent_id = ResourceIdType()

    Options = get_model_options(public_api_fields=(
        "id",
        "name",
        "parent_id",
    ))

    @classmethod
    def db_fields(cls, table_name=""):
        if table_name:
            table_name += "."

        return ",".join("{}{}".format(table_name, key) for key, _ in cls.fields.items())


class CategoryList(Model):
    next_page_token = ResourceIdType()
    categories = ListType(ModelType(CategoryShortView), default=list)
    Options = get_model_options(public_api_fields=("next_page_token", "categories"))


class CategoryListingRequest(BasePublicListingRequest):
    filter = StringType()


class CategoryCreateRequest(MktBasePublicModel):
    name = JsonDictType(StringType, required=True)
    type = StringEnumType(choices=Category.Type.ALL, required=True)
    score = FloatType(required=True, default=0.0)
    parent_id = ResourceIdType()


class CategoryUpdateRequest(MktBasePublicModel):
    id = ResourceIdType(required=True,
                        metadata={MetadataOptions.QUERY_VARIABLE: "category_id"})
    name = JsonDictType(StringType)
    type = StringEnumType(choices=Category.Type.ALL)
    score = FloatType()
    parent_id = ResourceIdType()


class CategoryMetadata(MktBasePublicModel):
    category_id = ResourceIdType()


class CategoryOperation(OperationV1Beta1):
    metadata = ModelType(CategoryMetadata)
    response = ModelType(CategoryResponse)


class CategoryRemoveResourcesRequest(MktBasePublicModel):
    category_id = ResourceIdType(required=True,
                                 metadata={MetadataOptions.QUERY_VARIABLE: "category_id"})
    resource_ids = ListType(ResourceIdType(), required=True)


class CategoryAddResourcesRequest(MktBasePublicModel):
    category_id = ResourceIdType(required=True,
                                 metadata={MetadataOptions.QUERY_VARIABLE: "category_id"})
    resource_ids = ListType(ResourceIdType(), required=True)
