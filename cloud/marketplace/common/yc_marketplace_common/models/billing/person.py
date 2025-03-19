from schematics.types import BooleanType
from schematics.types import ListType
from schematics.types import ModelType
from schematics.types import StringType

from yc_common.clients.models import BasePublicModel
from yc_common.models import get_model_options


class CompanyPersonData(BasePublicModel):
    name = StringType()
    longname = StringType()
    phone = StringType()
    email = StringType()
    post_code = StringType()
    post_address = StringType()
    legal_address = StringType()
    inn = StringType()
    kpp = StringType()
    bik = StringType()
    account = StringType()
    is_partner = BooleanType()

    Options = get_model_options(public_api_fields=(
        "name",
        "longname",
        "phone",
        "email",
        "post_code",
        "post_address",
        "legal_address",
        "inn",
        "kpp",
        "bik",
        "account",
        "is_partner",
    ))


class IndividualPersonData(BasePublicModel):
    last_name = StringType(metadata={"label": "Last Name"})
    phone = StringType(metadata={"label": "Phone"})
    first_name = StringType(metadata={"label": "First Name"})
    middle_name = StringType(metadata={"label": "Middle Name"})
    email = StringType(metadata={"label": "E-mail"})
    uid = StringType(metadata={"label": "UID"})
    is_partner = BooleanType()
    inn = StringType(metadata={"label": "INN"})
    pfr = StringType(metadata={"label": "snils"})

    Options = get_model_options(public_api_fields=(
        "last_name",
        "phone",
        "first_name",
        "middle_name",
        "email",
        "uid",
        "is_partner",
        "inn",
        "pfr",
    ))


class Person(BasePublicModel):
    class Type:
        INDIVIDUAL = "individual"
        COMPANY = "company"
        ALL = [INDIVIDUAL, COMPANY]

    id = StringType()
    type = StringType(choices=Type.ALL, required=True)

    def __new__(cls, raw_data: dict, *args, **kwargs):
        subclasses = {
            Person.Type.INDIVIDUAL: IndividualPerson,
            Person.Type.COMPANY: CompanyPerson
        }
        submodel = None
        if raw_data["type"] == Person.Type.INDIVIDUAL:
            submodel = Person.Type.INDIVIDUAL
        elif raw_data["type"] == Person.Type.COMPANY:
            submodel = Person.Type.COMPANY

        class_ = subclasses.get(submodel, cls) if submodel else cls
        return super().__new__(class_)


class CompanyPerson(Person):
    company = ModelType(CompanyPersonData, required=True)

    Options = get_model_options(public_api_fields=(
        "company",
    ))


class IndividualPerson(Person):
    individual = ModelType(IndividualPersonData, required=True)

    Options = get_model_options(public_api_fields=(
        "individual",
    ))


class PersonRequest(BasePublicModel):
    type = StringType(choices=Person.Type.ALL, required=True)
    company = ModelType(CompanyPersonData)
    individual = ModelType(IndividualPersonData)


class PrivatePersonRequest(BasePublicModel):
    passport_uid = StringType(required=True)


class PrivatePersonsList(BasePublicModel):
    passport_uid = StringType(required=True)
    persons = ListType(ModelType(Person), required=True)
