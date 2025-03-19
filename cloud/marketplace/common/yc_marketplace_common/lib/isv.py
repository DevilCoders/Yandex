from yc_common import config
from yc_common.paging import page_handler

from cloud.marketplace.common.yc_marketplace_common.db.models import isv_table
from cloud.marketplace.common.yc_marketplace_common.lib.partner import Partner
from cloud.marketplace.common.yc_marketplace_common.lib.task import TaskUtils
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.models.isv import Isv as IsvScheme
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvList
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvMetadata
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvOperation
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvRequest
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvResponse
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvUpdateStatusRequest
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerTypes
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidIsvIdError
from cloud.marketplace.common.yc_marketplace_common.utils.errors import IsvConflictError
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction


class Isv(Partner):
    default_category = "marketplace.default_isv_category"
    table = isv_table
    model = IsvScheme
    response_model = IsvResponse
    partner_type = PartnerTypes.ISV
    category_type = Category.Type.ISV
    error_not_found = InvalidIsvIdError
    error_conflict = IsvConflictError

    @staticmethod
    @mkt_transaction()
    @page_handler(items="isvs")
    def rpc_public_list(*args, **kwargs):
        return Isv._construct_isv_list(*Isv._rpc_list(*args, active=True, **kwargs))

    @staticmethod
    @mkt_transaction()
    @page_handler(items="isvs")
    def rpc_list(*args, **kwargs):
        return Isv._construct_isv_list(*Isv._rpc_list(*args, active=False, **kwargs))

    @staticmethod
    def _construct_isv_list(iterator, limit):
        isv_list = IsvList({
            "isvs": [isv.to_api(True) for isv in iterator],
        })

        if limit is not None and len(isv_list.isvs) == limit:
            isv_list.next_page_token = isv_list.isvs[-1].id

        return isv_list

    @staticmethod
    @mkt_transaction()
    def rpc_create(request: IsvRequest, request_context, *, tx, admin=False):
        req, isv = Isv._rpc_create(request, tx, admin=admin)

        return Isv.task_update(req, isv, request.billing_account_id, tx=tx)

    @staticmethod
    @mkt_transaction()
    def rpc_update(request: IsvUpdateRequest, request_context, *, tx, auth=None):
        id = request.isv_id
        op = IsvOperation
        metadata = {"isv_id": id}

        req, partner = Isv._rpc_update(request, id, op, metadata, tx, auth)

        return Isv.task_update(req, partner, request.billing_account_id, tx=tx)

    @staticmethod
    @mkt_transaction()
    def rpc_set_status(request: IsvUpdateStatusRequest, *, tx):
        # Can not check status because it is admin handle and we can do it always =)

        isv = Isv.rpc_get(request.isv_id, tx=tx)
        return Isv.task_set_status(isv, request.status)

    @staticmethod
    @mkt_transaction()
    def task_set_status(isv, status, *, tx):
        group = generate_id()

        return TaskUtils.create(
            operation_type="finalize_partner",
            group_id=group,
            params={
                "id": isv.id,
                "diff": {
                    "status": status,
                },
                "type": PartnerTypes.ISV,
            },
            metadata=IsvMetadata({"isv_id": isv.id}).to_primitive(),
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def task_update(request, isv, billing_account_id=None, tx=None) -> IsvOperation:
        check_task, group = Isv._task_update_start(request, tx)
        depends = [check_task.id]

        if billing_account_id:
            sync_task = TaskUtils.create(
                operation_type="sync_isv_to_billing",
                group_id=group,
                params={
                    "billing_account_id": billing_account_id,
                },
                depends=[check_task.id],
                metadata={},
                tx=tx,
            )

            depends.append(sync_task.id)

        s3_endpoint = config.get_value("endpoints.s3.publishers_bucket")  # TODO
        metadata = IsvMetadata({"isv_id": isv.id}).to_primitive()
        return Isv._task_update_finish(check_task, group, depends, request, isv, s3_endpoint, metadata, tx)
