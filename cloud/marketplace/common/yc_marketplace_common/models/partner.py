from schematics.types import StringType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from yc_common.clients.kikimr import KikimrDataType
from yc_common.fields import JsonDictType
from yc_common.models import IsoTimestampType
from yc_common.models import JsonListType
from yc_common.models import JsonModelType
from yc_common.models import StringEnumType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class _Status(object):
    PENDING = "pending"
    ACTIVE = "active"
    SUSPENDED = "suspended"
    ERROR = "error"

    ALL = (PENDING, ACTIVE, SUSPENDED, ERROR)
    PUBLIC = {ACTIVE}


class _ContactInfo(MktBasePublicModel):
    uri = StringType()
    phone = StringType()
    address = StringType()

    Options = get_model_options(public_api_fields=("uri", "phone", "address"))


class PartnerTypes:
    PUBLISHER = "publisher"
    ISV = "isv"
    VAR = "var"

    ALL = {ISV, PUBLISHER, VAR}


class PartnerInterface(AbstractMktBase):
    Status = _Status
    ContactInfo = _ContactInfo

    id = ResourceIdType(required=True)
    name = StringType(required=True)
    description = StringType(required=True)
    contact_info = JsonModelType(ContactInfo)
    logo_id = ResourceIdType(required=False)
    logo_uri = StringType(required=False)
    created_at = IsoTimestampType(required=True)
    meta = JsonDictType(StringType)
    billing_account_id = ResourceIdType(required=True)

    status = StringEnumType(required=True, choices=Status.ALL)

    Filterable_fields = {
        "id",
        "created_at",
        "name",
        "description",
        "status",
    }

    @classmethod
    def get_db_fields(cls):
        return {
            "id": KikimrDataType.UTF8,
            "name": KikimrDataType.UTF8,
            "description": KikimrDataType.UTF8,
            "contact_info": KikimrDataType.JSON,
            "logo_id": KikimrDataType.UTF8,
            "logo_uri": KikimrDataType.UTF8,
            "status": KikimrDataType.UTF8,
            "created_at": KikimrDataType.UINT64,
            "meta": KikimrDataType.JSON,
            "billing_account_id": KikimrDataType.UTF8,

        }

    @classmethod
    def _from_request(cls, request) -> "PartnerInterface":
        if request.logo_id is not None:
            return cls.new(
                billing_account_id=request.billing_account_id,
                name=request.name,
                description=request.description,
                contact_info=request.contact_info,
                logo_id=request.logo_id,
                meta=request.meta,
            )
        return cls.new(
            billing_account_id=request.billing_account_id,
            name=request.name,
            description=request.description,
            contact_info=request.contact_info,
            meta=request.meta,
        )


class PartnerResponseInterface(MktBasePublicModel):
    id = ResourceIdType(required=True)
    name = StringType(required=True)
    description = StringType(required=True)
    contact_info = JsonModelType(_ContactInfo)
    logo_id = ResourceIdType(required=False)
    logo_uri = StringType(required=False)
    created_at = IsoTimestampType(required=True)
    meta = JsonDictType(StringType)
    billing_account_id = ResourceIdType(required=True)

    categories = JsonListType(StringType)

    status = StringEnumType(required=True, choices=_Status.ALL)

    @classmethod
    def get_api_fields(cls):
        return [
            "id",
            "name",
            "description",
            "contact_info",
            "logo_id",
            "logo_uri",
            "status",
            "created_at",
            "meta",
            "categories",
            "billing_account_id",
        ]


class PartnerCreateRequest(MktBasePublicModel):
    name = StringType(required=True)
    description = StringType(required=True)
    contact_info = JsonModelType(_ContactInfo)
    logo_id = ResourceIdType(required=False)
    meta = JsonDictType(JsonDictType(StringType), default={})
    billing_account_id = ResourceIdType(required=True)


class PartnerUpdateRequest(MktBasePublicModel):
    name = StringType(required=False)
    description = StringType(required=False)
    contact_info = JsonModelType(_ContactInfo, required=False)
    logo_id = ResourceIdType(required=False)
    meta = JsonDictType(JsonDictType(StringType), default={})
    billing_account_id = ResourceIdType(required=True)
