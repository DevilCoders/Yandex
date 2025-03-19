from schematics.types import IntType
from schematics.types import ListType
from schematics.types import ModelType
from schematics.types import StringType
from yc_common.clients.models import BasePublicModel
from yc_common.clients.models.base import BaseListModel
from yc_common.models import NormalizedDecimalType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class GrantPolicyPublicView(BasePublicModel):
    id = ResourceIdType(required=True)
    start_time = IntType(required=False)
    end_time = IntType(required=False)
    paid_start_time = IntType(required=False)
    paid_end_time = IntType(required=False)
    rate = NormalizedDecimalType(required=True)
    aggregation_span = IntType(required=True)
    grant_duration = IntType(required=True)
    max_count = IntType(required=False)
    max_amount = NormalizedDecimalType(required=False)
    name = StringType(required=True)

    created_at = IntType(required=True)

    Options = get_model_options(public_api_fields=(
        "id",
        "start_time",
        "end_time",
        "paid_start_time",
        "paid_end_time",
        "rate",
        "aggregation_span",
        "grant_duration",
        "name",
        "max_count",
        "max_amount",
        "created_at",
    ))


class GrantPolicyList(BaseListModel):
    policies = ListType(ModelType(GrantPolicyPublicView), default=[])

    Options = get_model_options(public_api_fields=(
        "policies",
        "next_page_token"
    ))


class GrantPolicyCreateRequest(BasePublicModel):
    start_time = IntType(required=False)
    end_time = IntType(required=False)
    paid_start_time = IntType(required=False)
    paid_end_time = IntType(required=False)
    rate = NormalizedDecimalType(required=True)
    aggregation_span = IntType(required=True)
    grant_duration = IntType(required=True)
    max_count = IntType(required=False, min_value=1)
    max_amount = NormalizedDecimalType(required=False)
    name = StringType(required=True)
