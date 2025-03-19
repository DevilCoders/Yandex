from schematics.types import BooleanType
from schematics.types import ListType
from schematics.types import ModelType
from schematics.types import StringType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.clients.models.base import BasePublicListingRequest
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.fields import JsonListType
from yc_common.misc import timestamp
from yc_common.models import IsoTimestampType
from yc_common.models import JsonSchemalessDictType
from yc_common.models import MetadataOptions
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType

FIELD_CONTROLS = {"input", "textarea"}


class Field(MktBasePublicModel):
    field = StringType(required=True)
    control = StringType(required=True, choices=FIELD_CONTROLS)
    label = StringType()
    placeholder = StringType()
    hint = StringType()
    default = StringType()

    Options = get_model_options(public_api_fields=("field", "control", "label", "placeholder", "hint", "default"))


class FormResponse(MktBasePublicModel):
    id = ResourceIdType(required=True)
    title = StringType(required=True)
    billing_account_id = ResourceIdType(required=True)
    metadata_template = StringType(required=True)
    public = BooleanType(required=True)
    fields = ListType(ModelType(Field), required=True)
    schema = JsonSchemalessDictType(required=True)

    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType(required=True)

    Options = get_model_options(public_api_fields=("id", "fields", "schema", "metadata_template"))


class FormList(MktBasePublicModel):
    next_page_token = StringType()
    forms = ListType(ModelType(FormResponse), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "forms"))


class FormCreateRequest(MktBasePublicModel):
    title = StringType(required=True)
    billing_account_id = ResourceIdType(required=True)
    metadata_template = StringType(required=True)
    fields = ListType(ModelType(Field), required=True)
    schema = JsonSchemalessDictType(required=True)


class FormUpdateRequest(MktBasePublicModel):
    id = ResourceIdType(required=True, metadata={MetadataOptions.QUERY_VARIABLE: "form_id"})
    title = StringType()
    metadata_template = StringType()
    fields = ListType(ModelType(Field))
    schema = JsonSchemalessDictType()


class FormListingRequest(BasePublicListingRequest):
    filter = StringType()
    order_by = StringType()


class FormMetadata(MktBasePublicModel):
    form_id = ResourceIdType(required=True)


class FormOperation(OperationV1Beta1):
    metadata = ModelType(FormMetadata, required=True)
    response = ModelType(FormResponse)


class FormScheme(AbstractMktBase):
    id = ResourceIdType(required=True)
    title = StringType(required=True)
    billing_account_id = ResourceIdType(required=True)
    metadata_template = StringType(required=True)
    public = BooleanType(required=True)
    items = JsonListType(JsonSchemalessDictType(), required=True, serialized_name="fields")
    schema = JsonSchemalessDictType(required=True)

    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType(required=True)

    Filterable_fields = {
        "id",
        "billing_account_id",
        "public",
        "title",
    }

    PublicModel = FormResponse

    data_model = DataModel(
        [Table(
            name="form",
            spec=KikimrTableSpec(
                columns={
                    "id": KikimrDataType.UTF8,
                    "title": KikimrDataType.UTF8,
                    "billing_account_id": KikimrDataType.UTF8,
                    "public": KikimrDataType.BOOL,
                    "metadata_template": KikimrDataType.UTF8,
                    "fields": KikimrDataType.JSON,
                    "schema": KikimrDataType.JSON,
                    "created_at": KikimrDataType.UINT64,
                    "updated_at": KikimrDataType.UINT64,
                },
                primary_keys=["id"],
            ),
        )],
    )

    @classmethod
    def from_create_request(cls, request: "FormCreateRequest"):
        return cls.new(
            id=generate_id(),
            title=request.title,
            billing_account_id=request.billing_account_id,
            public=True,
            metadata_template=request.metadata_template,
            items=[f.to_kikimr() for f in request.fields],
            schema=request.schema,
            created_at=timestamp(),
            updated_at=timestamp(),
        )
