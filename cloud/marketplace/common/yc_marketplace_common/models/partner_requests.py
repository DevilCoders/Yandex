from schematics.types import ListType
from schematics.types import ModelType
from schematics.types import StringType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerTypes
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.clients.models.base import BasePublicListingRequest
from yc_common.misc import timestamp
from yc_common.models import IsoTimestampType
from yc_common.models import JsonSchemalessDictType
from yc_common.models import StringEnumType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class _Status:
    APPROVE = "approve"
    DECLINE = "reject"
    NONE = "none"
    CLOSE = "close"

    ALL = {NONE, CLOSE, APPROVE, DECLINE}
    DONE = {CLOSE, APPROVE, DECLINE}


class PartnerRequestPublicCreate(MktBasePublicModel):
    diff = JsonSchemalessDictType(required=True)
    partner_id = ResourceIdType(required=True)
    type = StringType(required=True, choices=PartnerTypes.ALL)
    Options = get_model_options(public_api_fields=(
        "partner_id",
        "type",
        "diff",
    ))


class PartnerRequestPublic(MktBasePublicModel):
    id = ResourceIdType(required=True)
    status = StringEnumType(required=True, choices=_Status.ALL)
    diff = JsonSchemalessDictType(required=True)
    partner_id = ResourceIdType(required=True)
    created_at = IsoTimestampType(required=True)
    type = StringType(choices=PartnerTypes.ALL)

    Options = get_model_options(public_api_fields=(
        "id",
        "partner_id",
        "diff",
        "type",
        "status",
        "created_at",
    ))


class PartnerRequestPublicList(MktBasePublicModel):
    next_page_token = StringType()
    partner_requests = ListType(ModelType(PartnerRequestPublic), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "partner_requests"))


class PartnerRequest(AbstractMktBase):
    Status = _Status
    PublicModel = PartnerRequestPublic

    id = ResourceIdType(required=True)
    type = StringType(required=True, choices=PartnerTypes.ALL)
    status = StringEnumType(required=True, choices=_Status.ALL)
    diff = JsonSchemalessDictType(required=True)
    partner_id = ResourceIdType(required=True)
    created_at = IsoTimestampType(required=True)

    data_model = DataModel((
        Table(
            name="partner_requests", spec=KikimrTableSpec(
                columns={
                    "id": KikimrDataType.UTF8,
                    "partner_id": KikimrDataType.UTF8,
                    "diff": KikimrDataType.JSON,
                    "type": KikimrDataType.UTF8,
                    "status": KikimrDataType.UTF8,
                    "created_at": KikimrDataType.UINT64,
                },
                primary_keys=["id"],
            ),
        ),
    ))

    Filterable_fields = {
        "id",
        "partner_id",
        "status",
        "type",
        "created_at",
    }

    @classmethod
    def create(cls, *args, **kwargs):
        return super().new(
            id=generate_id(),
            created_at=timestamp(),
            status=_Status.NONE,
            *args,
            **kwargs,
        )

    @classmethod
    def fake(cls, status, *args, **kwargs):
        return super().new(
            id=generate_id(),
            created_at=timestamp(),
            status=status,
            *args,
            **kwargs,
        )


class PartnerRequestsListingRequest(BasePublicListingRequest):
    # order_by = StringType()
    filter = StringType()
