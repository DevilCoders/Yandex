from yc_common.clients.models import BasePublicModel
from yc_common.fields import BooleanType
from yc_common.fields import DateTimeType
from yc_common.fields import ListType
from yc_common.fields import ModelType
from yc_common.fields import StringType
from yc_common.formatting import camelcase_to_underscore
from yc_common.formatting import underscore_to_lowercamelcase
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

class SwitzerlandCompanyPersonData(BasePublicModel):
    name = StringType()
    phone = StringType()
    email = StringType()
    post_code = StringType()
    post_address = StringType()


class Person(BasePublicModel):
    class Type:
        # Russian currency and tax residents
        INDIVIDUAL = "individual"
        COMPANY = "company"

        # other
        SWITZERLAND_RESIDENT_COMPANY = "switzerland_resident_company"
        SWITZERLAND_NONRESIDENT_COMPANY = "switzerland_nonresident_company"
        RUSSIAN_RESIDENTS = [INDIVIDUAL, COMPANY]
        INVOICEABLE = [COMPANY, SWITZERLAND_RESIDENT_COMPANY, SWITZERLAND_NONRESIDENT_COMPANY]
        ALL = [INDIVIDUAL, COMPANY, SWITZERLAND_RESIDENT_COMPANY, SWITZERLAND_NONRESIDENT_COMPANY]

    id = StringType()
    is_partner = BooleanType(required=True, default=False)
    created_at = DateTimeType()
    type = StringType(choices=[underscore_to_lowercamelcase(type_) for type_ in Type.ALL] + Type.ALL,
                      required=True)
    display_name = StringType()
    Options = get_model_options(public_api_fields=(
        "client_id",
        "type",
        "individual",
      	"display_name",
        "is_partner",
        "company",
        "switzerland_resident_company",
        "switzerland_nonresident_company",
    ))

    @property
    def is_russian_resident(self):
        return type(self) in (CompanyPerson, IndividualPerson)

    @property
    def is_legal_person(self):
        return type(self) in (CompanyPerson, SwitzerlandNonResidentCompanyPerson, SwitzerlandResidentCompanyPerson)

    def __new__(cls, raw_data: dict, *args, **kwargs):
        subclasses = {
            Person.Type.INDIVIDUAL: IndividualPerson,
            Person.Type.COMPANY: CompanyPerson,
            Person.Type.SWITZERLAND_RESIDENT_COMPANY: SwitzerlandResidentCompanyPerson,
            Person.Type.SWITZERLAND_NONRESIDENT_COMPANY: SwitzerlandNonResidentCompanyPerson
        }
        # type value is camelcased
        type_ = camelcase_to_underscore(raw_data["type"])
        kwargs["type"] = type_
        submodel = type_ if type_ in subclasses else None
        class_ = subclasses.get(submodel, cls) if submodel else cls
        return super().__new__(class_)

    def __eq__(self, other):
        if self.type != other.type:
            return False
        if self.balance_client_id != other.balance_client_id:
            return False

        if self.type == Person.Type.COMPANY:
            return self.company == other.company
        if self.type == Person.Type.INDIVIDUAL:
            return self.individual == other.individual
        if self.type == Person.Type.SWITZERLAND_NONRESIDENT_COMPANY:
            return self.switzerland_nonresident_company == other.switzerland_nonresident_company
        if self.type == Person.Type.SWITZERLAND_RESIDENT_COMPANY:
            return self.switzerland_resident_company == other.switzerland_resident_company
        raise NotImplementedError

    def to_primitive(self, role=None, app_data=None, **kwargs):
        result = super().to_primitive(role=role, app_data=app_data, **kwargs)
        result["type"] = underscore_to_lowercamelcase(result["type"])
        return result


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


class SwitzerlandResidentCompanyPerson(Person):
    switzerland_resident_company = ModelType(SwitzerlandCompanyPersonData, required=True)

    Options = get_model_options(public_api_fields=(
        "switzerland_resident_company",
    ))


class SwitzerlandNonResidentCompanyPerson(Person):
    switzerland_nonresident_company = ModelType(SwitzerlandCompanyPersonData, required=True)

    Options = get_model_options(public_api_fields=(
        "switzerland_nonresident_company",
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
