from schematics.types import ListType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.models import ModelType
from yc_common.models import StringType
from yc_common.models import get_model_options


class OperationList(MktBasePublicModel):
    next_page_token = StringType()
    operations = ListType(ModelType(OperationV1Beta1), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "operations"))
