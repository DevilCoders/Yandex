from schematics.types import BooleanType
from schematics.types import IntType
from schematics.types import ListType
from schematics.types import StringType
from schematics.types import DateTimeType

from yc_common.clients.models import BasePublicModel
from yc_common.models import get_model_options


class Contract(BasePublicModel):
    class Type:
        GENERAL = "general"
        OFFER = "offer"
        SPENDABLE = "spendable"
        ALL = [GENERAL, OFFER, SPENDABLE]

    class PaymentType:
        PREPAY = "prepay"
        POSTPAY = "postpay"
        ALL = [PREPAY, POSTPAY]

    class PaymentCycleType:
        MONTHLY = "monthly"
        QUARTER = "quarter"

        ALL = [MONTHLY, QUARTER]

    balance_client_id = StringType()
    is_suspended = BooleanType()
    is_active = BooleanType()
    services = ListType(IntType)
    project_ids = ListType(StringType())
    person_id = StringType()
    payment_type = StringType(choices=PaymentType.ALL)
    payment_cycle_type = StringType(choices=PaymentCycleType.ALL)
    is_faxed = BooleanType()
    is_signed = BooleanType()
    is_deactivated = BooleanType()
    is_cancelled = BooleanType()
    id = StringType()
    external_id = StringType()
    effective_date = DateTimeType()
    currency = StringType()
    type = StringType(choices=Type.ALL)
    manager_code = StringType()

    Options = get_model_options(
        public_api_fields=(
            "balance_client_id",
            "is_suspended",
            "is_active",
            "services",
            "project_ids",
            "person_id",
            "payment_type",
            "payment_cycle_type",
            "is_faxed",
            "is_signed",
            "is_deactivated",
            "is_cancelled",
            "id",
            "external_id",
            "effective_date",
            "currency",
            "type",
            "manager_code",
        )
    )
