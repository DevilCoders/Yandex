from schematics.types import DateType
from schematics.types import IntType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.billing.contract import Contract
from cloud.marketplace.common.yc_marketplace_common.models.billing.person import Person
from cloud.marketplace.common.yc_marketplace_common.models.billing.person import PersonRequest
from cloud.marketplace.common.yc_marketplace_common.models.billing.publisher_account import VAT
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerInterface
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerResponseInterface
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from yc_common.clients.kikimr import KikimrDataType
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


class PublisherResponse(PartnerResponseInterface):
    billing_publisher_account_id = ResourceIdType()
    passport_uid = IntType(min_value=1)  # юид инициатора - не возвращаем в АПИ

    Options = get_model_options(
        public_api_fields=(["billing_publisher_account_id"] + PartnerResponseInterface.get_api_fields()))


class PublisherFullResponse(PublisherResponse):
    person = ModelType(Person)
    contract = ModelType(Contract)

    Options = get_model_options(
        public_api_fields=(["person", "contract"] + PublisherResponse.get_api_fields()))


class Publisher(PartnerInterface):
    @property
    def PublicModel(self):
        return PublisherResponse

    billing_publisher_account_id = ResourceIdType()
    passport_uid = IntType(min_value=1)

    data_model = DataModel((
        Table(
            name="publishers", spec=KikimrTableSpec(
                columns=dict(
                    billing_publisher_account_id=KikimrDataType.UTF8,
                    passport_uid=KikimrDataType.UINT64,
                    **PartnerInterface.get_db_fields(),
                ),
                primary_keys=["id"],
            ),
        ),
    ))

    @classmethod
    def new(cls, status=PartnerInterface.Status.PENDING, **kwargs) -> "Publisher":
        return super().new(status=status, **kwargs)

    @classmethod
    def from_request(cls, request) -> "Publisher":
        return cls._from_request(request)


class PublisherList(MktBasePublicModel):
    next_page_token = StringType()
    publishers = ListType(ModelType(PublisherResponse), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "publishers"))


class PublisherRequest(PartnerCreateRequest):
    billing = ModelType(PersonRequest, required=True)
    passport_uid = IntType(min_value=1)  # юид инициатора
    vat = StringType(required=True, choices=VAT.ALL, default=VAT.RU_DEFAULT)


class PublisherUpdateRequest(PartnerUpdateRequest):
    billing = ModelType(PersonRequest, default=None)
    passport_uid = IntType(min_value=1)  # юид инициатора
    publisher_id = ResourceIdType(required=True, metadata={MetadataOptions.QUERY_VARIABLE: "publisher_id"})


class PublisherUpdateStatusRequest(MktBasePublicModel):
    publisher_id = ResourceIdType(required=True, metadata={MetadataOptions.QUERY_VARIABLE: "publisher_id"})
    status = StringEnumType(required=True, choices=PartnerInterface.Status.ALL)


class PublisherMetadata(MktBasePublicModel):
    publisher_id = ResourceIdType(required=True)


class PublisherOperation(OperationV1Beta1):
    metadata = JsonModelType(PublisherMetadata, required=True)
    response = JsonModelType(PublisherResponse)


class PublisherRevenueReportRequest(MktBasePublicModel):
    publisher_id = ResourceIdType(required=True, metadata={MetadataOptions.QUERY_VARIABLE: "publisher_id"})
    start_date = DateType(required=True)
    end_date = DateType(required=True)
    sku_ids = ListType(ResourceIdType, required=False, default=list)
