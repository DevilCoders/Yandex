from schematics.types import IntType
from schematics.types import ModelType
from schematics.types import StringType
from yc_common.clients.models import BasePublicModel
from yc_common.clients.models.billing.person import Person
from yc_common.clients.models.billing.person import PersonRequest
from yc_common.clients.models.billing.contract import Contract
from yc_common.validation import ResourceIdType


class VAT:
    RU_DEFAULT = "ru_default"  # 20%
    RU_NO_VAT = "ru_no_vat"  # 0

    ALL = [RU_DEFAULT, RU_NO_VAT]


class PublisherAccount(BasePublicModel):
    class State:
        NEW = "new"
        ACTIVE = "active"
        INACTIVE = "inactive"
        DELETED = "deleted"

        ALL = [NEW, ACTIVE, INACTIVE, DELETED]

    class PaymentCycleType:
        MONTHLY = "monthly"
        QUARTER = "quarter"

        ALL = [MONTHLY, QUARTER]

    id = ResourceIdType(required=True)
    owner_id = ResourceIdType(required=True)

    name = StringType(required=True, default="default account")

    payment_cycle_type = StringType(required=True, default=PaymentCycleType.MONTHLY, choices=PaymentCycleType.ALL)

    balance_client_id = StringType()
    balance_person_id = StringType()
    balance_person_type = StringType()
    balance_contract_id = StringType()

    currency = StringType(required=True, default="RUB")
    state = StringType(default=State.NEW, choices=State.ALL, required=True)
    created_at = IntType(required=True, metadata={"label": "Publisher's creation ts"})
    updated_at = IntType(required=True, metadata={"label": "Last publisher's update ts"})

    # Full view fields
    person = ModelType(Person)
    contract = ModelType(Contract)


class PublisherAccountCreateRequest(BasePublicModel):
    passport_uid = StringType(required=True)
    marketplace_publisher_id = ResourceIdType(required=True)
    name = StringType(required=True)
    currency = StringType()  # IBAN CURRENCY CODE
    vat = StringType(required=True, choices=VAT.ALL, default=VAT.RU_DEFAULT)

    person_data = ModelType(PersonRequest)


class PublisherAccountUpdateRequest(BasePublicModel):
    publisher_account_id = ResourceIdType()
    name = StringType()
    person_id = StringType()
    person_data = ModelType(PersonRequest)


class PublisherAccountGetRequest(BasePublicModel):
    class ViewType:
        SHORT = "short"
        FULL = "full"

        ALL = [SHORT, FULL]

    view = StringType(choices=ViewType.ALL, required=True, default=ViewType.SHORT)
