from schematics.types import BooleanType
from schematics.types import IntType
from schematics.types import ListType
from schematics.types import ModelType
from schematics.types import StringType
from yc_common.clients.models import BasePublicModel
from yc_common.fields import SchemalessDictType
from yc_common.models import JsonDictType
from yc_common.models import NormalizedDecimalType
from yc_common.validation import ResourceIdType

from cloud.marketplace.common.yc_marketplace_common.models.billing.contract import Contract
from cloud.marketplace.common.yc_marketplace_common.models.billing.person import Person


class UnblockReasons:
    DEBT_PAID = "debt_paid"
    THRESHOLD_PAID = "threshold_paid"
    VERIFICATION = "verification"
    MANUAL = "manual"

    ALL = [MANUAL, DEBT_PAID, THRESHOLD_PAID, VERIFICATION]


class BlockReasons:
    MANUAL = "manual"
    THRESHOLD = "threshold"
    TRIAL_EXPIRED = "trial_expired"
    DEBT = "debt"
    UNBOUND_CARD = "unbound_card"
    MINING = "mining"

    ALL = [MANUAL, THRESHOLD, DEBT, TRIAL_EXPIRED, UNBOUND_CARD, MINING]


class Type:
    SELF_SERVED = "self-served"
    INVOICED = "invoiced"
    MARKETPLACE = "marketplace"

    ALL = [SELF_SERVED, INVOICED, MARKETPLACE]


class UsageStatus:
    TRIAL = "trial"
    PAID = "paid"
    SERVICE = "service"
    DISABLED = "disabled"

    ALL = [PAID, SERVICE, TRIAL, DISABLED]
    FREE = [SERVICE, TRIAL, DISABLED]


class PaymentCycleType:
    MONTHLY = "monthly"
    THRESHOLD = "threshold"

    ALL = [MONTHLY, THRESHOLD]


class PaymentType:
    CARD = "card"
    INVOICE = "invoice"

    ALL = [CARD, INVOICE]


class State:
    NEW = "new"
    PAYMENT_NOT_CONFIRMED = "payment_not_confirmed"
    ACTIVE = "active"
    PAYMENT_REQUIRED = "payment_required"
    SUSPENDED = "suspended"
    INACTIVE = "inactive"
    DELETED = "deleted"

    ALL = [NEW, PAYMENT_NOT_CONFIRMED, PAYMENT_REQUIRED, ACTIVE, SUSPENDED, INACTIVE, DELETED]
    ALIVE_STATES = [NEW, PAYMENT_NOT_CONFIRMED, ACTIVE, SUSPENDED]


class DisplayStatus:
    NEW = "NEW"
    TRIAL_ACTIVE = "TRIAL_ACTIVE"
    TRIAL_SUSPENDED = "TRIAL_SUSPENDED"
    TRIAL_EXPIRED = "TRIAL_EXPIRED"
    SUSPENDED = "SUSPENDED"
    FIRST_PAYMENT_REQUIRED = "FIRST_PAYMENT_REQUIRED"
    PAYMENT_NOT_CONFIRMED = "PAYMENT_NOT_CONFIRMED"
    PAYMENT_REQUIRED = "PAYMENT_REQUIRED"
    ACTIVE = "ACTIVE"
    INACTIVE = "INACTIVE"
    SERVICE = "SERVICE"
    UNKNOWN = "UNKNOWN"


class BillingAccountFeatureFlags:
    ISV = "isv"
    VAR = "var"

    ALL = {ISV, VAR}


class BillingAccountFeatureRequest(BasePublicModel):
    feature_flag = StringType(required=True, choices=BillingAccountFeatureFlags.ALL)


class BillingAccount(BasePublicModel):
    id = ResourceIdType(required=True)
    client_id = ResourceIdType(required=True)
    name = StringType(required=True)
    type = StringType(required=True, choices=Type.ALL)
    usage_status = StringType(required=True, choices=UsageStatus.ALL)
    payment_method_id = ResourceIdType()
    payment_cycle_type = StringType(required=True, choices=PaymentCycleType.ALL)
    payment_threshold = StringType(required=False)
    balance = StringType(required=True)
    person_id = StringType(required=True)  # person_id in balance
    balance_contract_id = StringType()
    currency = StringType(required=True)
    state = StringType(required=True, choices=State.ALL)
    created_at = IntType(required=True)
    updated_at = IntType(required=True)
    feature_flags = JsonDictType(BooleanType, key=StringType(choices=BillingAccountFeatureFlags.ALL))


class CardPaymentMethod(BasePublicModel):
    """"""
    id = StringType(required=True)
    account = StringType(required=True)
    currency = StringType(required=True)
    holder = StringType(required=True)
    expired = BooleanType(required=True)
    expiration_month = IntType(required=True)
    expiration_year = IntType(required=True)
    pay_system = StringType(required=True)
    charge_limit = IntType(required=True)
    uid = StringType(required=True)


class BillingMetadataPrivateView(BasePublicModel):
    # paysystem
    autopay_failures = IntType(default=0)

    # idempotency && grants issuing
    verified = BooleanType(default=False)
    floating_threshold = BooleanType(default=True)
    idempotency_checks = ListType(StringType)
    fraud_detected_by = ListType(StringType)
    registration_ip = StringType()
    registration_user_iam = StringType()
    block_reason = StringType()
    unblock_reason = StringType()
    auto_grant_policies = ListType(StringType)
    paid_at = IntType()

    def __init__(self, *args, strict=False, **kwargs):
        super().__init__(*args, **kwargs, strict=False)


class BillingAccountFullView(BasePublicModel):
    id = ResourceIdType(required=True)
    name = StringType(required=True, default="default account")
    type = StringType(required=True, choices=Type.ALL)

    payment_type = StringType(required=True, choices=PaymentType.ALL)

    usage_status = StringType(required=True, choices=UsageStatus.ALL)
    payment_method_id = StringType(serialize_when_none=True)

    billing_threshold = NormalizedDecimalType(required=False)
    balance = NormalizedDecimalType(required=True, default="0")
    # person_id in balance
    person_id = StringType()
    master_account_id = ResourceIdType()
    person_type = StringType(choices=Person.Type.ALL)
    balance_contract_id = StringType()
    currency = StringType(required=True, default="RUB")
    display_status = StringType(required=True)
    balance_client_id = StringType()
    owner_id = StringType(required=True)
    state = StringType(default=State.NEW, choices=State.ALL, required=True)
    created_at = IntType(required=True, metadata={"label": "Client's creation ts"})
    updated_at = IntType(required=True, metadata={"label": "Last client's update ts"})
    feature_flags = JsonDictType(BooleanType, key=StringType(choices=BillingAccountFeatureFlags.ALL))
    person = ModelType(Person)
    contract = ModelType(Contract)
    payment_method = ModelType(CardPaymentMethod)
    is_var = BooleanType(default=False)
    passport = SchemalessDictType()
    metadata = ModelType(BillingMetadataPrivateView, required=False)
