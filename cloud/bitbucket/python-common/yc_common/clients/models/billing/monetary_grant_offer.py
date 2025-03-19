from schematics import Model
from schematics.types import IntType
from schematics.types import StringType
from yc_common.clients.models import BasePublicModel
from yc_common.models import JsonSchemalessDictType
from yc_common.validation import ResourceIdType


class MonetaryGrantOfferPublicView(BasePublicModel):
    id = ResourceIdType(required=True)
    initial_amount = StringType(required=True)
    duration = IntType(required=True)
    proposed_to = StringType()
    proposed_meta = JsonSchemalessDictType()
    passport_uid = StringType()
    created_at = IntType(required=True)


class BindMonetaryGrantOfferRequest(Model):
    passport_uid = StringType(required=True)
