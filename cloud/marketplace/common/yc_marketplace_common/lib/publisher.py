from yc_common import config
from yc_common.api.request_context import RequestContext
from yc_common.clients.billing import PublisherAccountCreateRequest
from yc_common.paging import page_handler

from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.client.billing import BillingPrivateClient
from cloud.marketplace.common.yc_marketplace_common.db.models import publishers_table
from cloud.marketplace.common.yc_marketplace_common.models.billing.publisher_account import PublisherAccountGetRequest
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerTypes
from cloud.marketplace.common.yc_marketplace_common.models.publisher import Publisher as PublisherScheme
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherFullResponse
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherList
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherMetadata
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherOperation
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherRequest
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherResponse
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherUpdateStatusRequest
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidPublisherIdError
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidStatus
from cloud.marketplace.common.yc_marketplace_common.utils.errors import PublisherConflictError
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.metadata_token import get_instance_metadata_token
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction


class Publisher(lib.Partner):
    default_category = "marketplace.default_publisher_category"
    table = publishers_table
    model = PublisherScheme
    response_model = PublisherResponse
    partner_type = PartnerTypes.PUBLISHER
    category_type = Category.Type.PUBLISHER
    error_not_found = InvalidPublisherIdError
    error_conflict = PublisherConflictError

    @staticmethod
    @mkt_transaction()
    def rpc_get_by_publisher_account(publisher_account_id, *, tx, auth=None):
        return Publisher._get([["billing_publisher_account_id = ?", publisher_account_id]], tx=tx, auth=auth)

    @staticmethod
    @mkt_transaction()
    @page_handler(items="publishers")
    def rpc_public_list(*args, **kwargs):
        return Publisher._construct_publisher_list(*Publisher._rpc_list(*args, active=True, **kwargs))

    @staticmethod
    @mkt_transaction()
    @page_handler(items="publishers")
    def rpc_list(*args, **kwargs):
        return Publisher._construct_publisher_list(*Publisher._rpc_list(*args, active=False, **kwargs))

    @staticmethod
    @mkt_transaction()
    def rpc_get_full(publisher_id, *, tx, auth=None):
        publisher = Publisher._get([["id = ?", publisher_id]], tx=tx, auth=auth)
        full_response = PublisherFullResponse(publisher)
        endpoint = config.get_value("endpoints.billing.url")
        billing_publisher_account = lib.Billing.get_publisher(endpoint, publisher.billing_publisher_account_id)
        full_response.person = billing_publisher_account.person
        full_response.contract = billing_publisher_account.contract
        return full_response

    @staticmethod
    def _construct_publisher_list(iterator, limit):
        publishers_list = PublisherList({
            "publishers": [publisher.to_api(True) for publisher in iterator],
        })
        if limit is not None and len(publishers_list.publishers) == limit:
            publishers_list.next_page_token = publishers_list.publishers[-1].id

        return publishers_list

    @staticmethod
    @mkt_transaction()
    def rpc_create(request: PublisherRequest, request_context: RequestContext, *, tx, admin=False):
        req, publisher = Publisher._rpc_create(request, tx, admin=admin)
        if request.passport_uid:
            passport = request.passport_uid
        elif request_context.auth:
            passport = request_context.auth.user.passport_uid
        else:
            passport = None

        billing = PublisherAccountCreateRequest.new(
            passport_uid=passport,
            marketplace_publisher_id=publisher.id,
            name=request.name,
            person_data=request.billing,
            vat=request.vat,
        )

        return Publisher.task_update(req, publisher, billing, tx=tx)

    @classmethod
    @mkt_transaction()
    def rpc_update(cls, request, request_context, *, tx, auth=None):
        id = request.publisher_id
        op = PublisherOperation
        metadata = {"publisher_id": id}

        req, partner = Publisher._rpc_update(request, id, op, metadata, tx, auth)

        if request.passport_uid:
            passport = request.passport_uid
        elif request_context.auth:
            passport = request_context.auth.user.passport_uid
        else:
            passport = None

        billing = PublisherAccountCreateRequest.new(
            passport_uid=passport,
            marketplace_publisher_id=id,
            name=request.name,
            person_data=request.billing,
        )

        return Publisher.task_update(req, partner, billing, tx=tx)

    @staticmethod
    @mkt_transaction()
    def rpc_set_status(request: PublisherUpdateStatusRequest, *, tx):
        # Can not check status because it is admin handle and we can do it always =)

        publisher = Publisher.rpc_get(request.publisher_id, tx=tx)
        return Publisher.task_set_status(publisher, request.status)

    @staticmethod
    @mkt_transaction()
    def task_set_status(publisher, status, *, tx):
        group = generate_id()

        # TODO rm acc from billing

        return lib.TaskUtils.create(
            operation_type="finalize_partner",
            group_id=group,
            params={
                "id": publisher.id,
                "diff": {
                    "status": status,
                },
                "type": PartnerTypes.PUBLISHER,
            },
            metadata=PublisherMetadata({"publisher_id": publisher.id}).to_primitive(),
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def task_update(request, publisher, billing, tx) -> PublisherOperation:
        check_task, group = Publisher._task_update_start(request, tx)
        depends = [check_task.id]
        if billing.person_data is not None:
            start_sync_task = lib.TaskUtils.create(
                operation_type="sync_publisher_to_billing_start",
                group_id=group,
                params={
                    "id": publisher.id,
                    "billing": billing.to_kikimr(),
                },
                depends=[],
                metadata={},
                tx=tx,
            )

            sync_task_finish = lib.TaskUtils.create(
                operation_type="sync_publisher_to_billing_finish",
                group_id=group,
                params={"id": publisher.id},
                metadata={},
                depends=[start_sync_task.id],
                tx=tx,
            )
            depends.append(sync_task_finish.id)

            lib.TaskUtils.create(
                operation_type="send_mail_to_docs",
                group_id=group,
                params={
                    "publisher_id": publisher.id,
                    # "from_addr": config.get_value(),
                },
                metadata={},
                depends=[sync_task_finish.id],
                tx=tx,
            )

            # depends.append(send_mail_task.id) TODO пусть сначала работает стабильно, потом зависеть будем

        s3_endpoint = config.get_value("endpoints.s3.publishers_bucket")
        metadata = PublisherMetadata({"publisher_id": publisher.id}).to_primitive()
        return Publisher._task_update_finish(check_task, group, depends, request, publisher, s3_endpoint, metadata, tx)

    @staticmethod
    @mkt_transaction()
    def account_signed(id, *, tx) -> bool:
        publisher = Publisher._get([["id = ?", id]], tx=tx)
        client = BillingPrivateClient(config.get_value("endpoints.billing.url"), iam_token=get_instance_metadata_token())
        request_data = PublisherAccountGetRequest({"view": PublisherAccountGetRequest.ViewType.FULL})
        account = client.get_publisher_account(publisher.billing_publisher_account_id, request_data)

        return account.contract.is_active

    @staticmethod
    @mkt_transaction()
    def check_state(billing_account_id, *, tx) -> bool:
        publisher = Publisher.rpc_get_by_ba(billing_account_id, tx=tx)
        if publisher.status != PublisherScheme.Status.ACTIVE:
            raise InvalidStatus()
