from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerInterface
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerResponseInterface
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.models import JsonModelType
from yc_common.models import ListType
from yc_common.models import MetadataOptions
from yc_common.models import ModelType
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class Isv(PartnerInterface):
    @property
    def PublicModel(self):
        return IsvResponse

    data_model = DataModel((
        Table(
            name="isv", spec=KikimrTableSpec(
                columns=PartnerInterface.get_db_fields(),
                primary_keys=["id"],
            ),
        ),
    ))

    @classmethod
    def new(cls, status=PartnerInterface.Status.PENDING, **kwargs) -> "Isv":
        return super().new(status=status, **kwargs)

    @classmethod
    def from_request(cls, request) -> "Isv":
        return cls._from_request(request)


class IsvResponse(PartnerResponseInterface):
    Options = get_model_options(public_api_fields=(
        PartnerResponseInterface.get_api_fields()
    ))


class IsvList(MktBasePublicModel):
    next_page_token = StringType()
    isvs = ListType(ModelType(IsvResponse), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "isvs"))


class IsvRequest(PartnerCreateRequest):
    pass


class IsvUpdateRequest(PartnerUpdateRequest):
    isv_id = ResourceIdType(required=True, metadata={MetadataOptions.QUERY_VARIABLE: "isv_id"})


class IsvUpdateStatusRequest(MktBasePublicModel):
    isv_id = ResourceIdType(required=True, metadata={MetadataOptions.QUERY_VARIABLE: "isv_id"})
    status = StringEnumType(required=True, choices=PartnerInterface.Status.ALL)


class IsvMetadata(MktBasePublicModel):
    isv_id = ResourceIdType(required=True)


class IsvOperation(OperationV1Beta1):
    metadata = JsonModelType(IsvMetadata, required=True)
    response = JsonModelType(IsvResponse)
