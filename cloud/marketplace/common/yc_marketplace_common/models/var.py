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


class Var(PartnerInterface):
    @property
    def PublicModel(self):
        return VarResponse

    data_model = DataModel((
        Table(
            name="var", spec=KikimrTableSpec(
                columns=PartnerInterface.get_db_fields(),
                primary_keys=["id"],
            ),
        ),
    ))

    @classmethod
    def new(cls, status=PartnerInterface.Status.PENDING, **kwargs) -> "Var":
        return super().new(status=status, **kwargs)

    @classmethod
    def from_request(cls, request) -> "Var":
        return cls._from_request(request)


class VarResponse(PartnerResponseInterface):
    Options = get_model_options(public_api_fields=(
        PartnerResponseInterface.get_api_fields()
    ))


class VarList(MktBasePublicModel):
    next_page_token = StringType()
    vars = ListType(ModelType(VarResponse), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "vars"))


class VarRequest(PartnerCreateRequest):
    pass


class VarUpdateRequest(PartnerUpdateRequest):
    var_id = ResourceIdType(required=True, metadata={MetadataOptions.QUERY_VARIABLE: "var_id"})


class VarUpdateStatusRequest(MktBasePublicModel):
    var_id = ResourceIdType(required=True, metadata={MetadataOptions.QUERY_VARIABLE: "var_id"})
    status = StringEnumType(required=True, choices=PartnerInterface.Status.ALL)


class VarMetadata(MktBasePublicModel):
    var_id = ResourceIdType(required=True)


class VarOperation(OperationV1Beta1):
    metadata = JsonModelType(VarMetadata, required=True)
    response = JsonModelType(VarResponse)
