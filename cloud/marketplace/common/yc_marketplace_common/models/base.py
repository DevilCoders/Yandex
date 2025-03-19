from schematics.types import StringType

from yc_common.clients.models.base import BasePublicListingRequest
from yc_common.validation import ResourceIdType


class BaseMktManageListingRequest(BasePublicListingRequest):
    billing_account_id = ResourceIdType(required=True)
    filter = StringType()
    order_by = StringType()


class BaseMktListingRequest(BasePublicListingRequest):
    filter = StringType()
    order_by = StringType()
