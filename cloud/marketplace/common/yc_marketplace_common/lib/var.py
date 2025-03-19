from flask.ctx import RequestContext

from yc_common import config
from yc_common.paging import page_handler
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import var_table
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerTypes
from cloud.marketplace.common.yc_marketplace_common.models.var import Var as VarScheme
from cloud.marketplace.common.yc_marketplace_common.models.var import VarList
from cloud.marketplace.common.yc_marketplace_common.models.var import VarMetadata
from cloud.marketplace.common.yc_marketplace_common.models.var import VarOperation
from cloud.marketplace.common.yc_marketplace_common.models.var import VarRequest
from cloud.marketplace.common.yc_marketplace_common.models.var import VarResponse
from cloud.marketplace.common.yc_marketplace_common.models.var import VarUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.models.var import VarUpdateStatusRequest
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidVarIdError
from cloud.marketplace.common.yc_marketplace_common.utils.errors import VarConflictError
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction


class Var(lib.Partner):
    default_category = "marketplace.default_var_category"
    table = var_table
    model = VarScheme
    response_model = VarResponse
    partner_type = PartnerTypes.VAR
    category_type = Category.Type.VAR
    error_not_found = InvalidVarIdError
    error_conflict = VarConflictError

    @staticmethod
    @mkt_transaction()
    @page_handler(items="vars")
    def rpc_public_list(*args, **kwargs):
        return Var._construct_var_list(*Var._rpc_list(*args, active=True, **kwargs))

    @staticmethod
    @mkt_transaction()
    @page_handler(items="vars")
    def rpc_list(*args, **kwargs):
        return Var._construct_var_list(*Var._rpc_list(*args, active=False, **kwargs))

    @staticmethod
    def _construct_var_list(iterator, limit):
        var_list = VarList({
            "vars": [var.to_api(True) for var in iterator],
        })

        if limit is not None and len(var_list.vars) == limit:
            var_list.next_page_token = var_list.vars[-1].id

        return var_list

    @staticmethod
    @mkt_transaction()
    def rpc_create(request: VarRequest, request_context: RequestContext, *, tx, admin=False):
        req, partner = Var._rpc_create(request, tx, admin=admin)

        return Var.task_update(req, partner, billing_account_id=request.billing_account_id, tx=tx)

    @staticmethod
    @mkt_transaction()
    def rpc_update(request: VarUpdateRequest, request_context, *, tx, auth=None):
        id = request.var_id
        op = VarOperation
        metadata = {"var_id": id}

        req, partner = Var._rpc_update(request, id, op, metadata, tx, auth)

        return Var.task_update(req, partner, billing_account_id=request.billing_account_id, tx=tx)

    @staticmethod
    @mkt_transaction()
    def rpc_set_status(request: VarUpdateStatusRequest, *, tx):
        # Can not check status because it is admin handle and we can do it always =)

        var = Var.rpc_get(request.var_id, tx=tx)
        return Var.task_set_status(var, request.status)

    @staticmethod
    @mkt_transaction()
    def task_set_status(var, status, *, tx):
        group = generate_id()

        return lib.TaskUtils.create(
            operation_type="finalize_partner",
            group_id=group,
            params={
                "id": var.id,
                "diff": {
                    "status": status,
                },
                "type": PartnerTypes.VAR,
            },
            depends=[],
            metadata=VarMetadata({"var_id": var.id}).to_primitive(),
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def task_update(request, var, billing_account_id=None, tx=None) -> VarOperation:
        check_task, group = Var._task_update_start(request, tx)
        depends = [check_task.id]

        if billing_account_id:
            start_sync_task = lib.TaskUtils.create(
                operation_type="sync_var_to_billing_start",
                group_id=group,
                params={
                    "billing_account_id": billing_account_id,
                },
                depends=[check_task.id],
                metadata={},
                tx=tx,
            )

            sync_task = lib.TaskUtils.create(
                operation_type="sync_var_to_billing_finish",
                group_id=group,
                params={},
                metadata={},
                depends=[start_sync_task.id],
                tx=tx,
            )
            depends.append(sync_task.id)

        s3_endpoint = config.get_value("endpoints.s3.publishers_bucket")  # TODO
        metadata = VarMetadata({"var_id": var.id}).to_primitive()
        return Var._task_update_finish(check_task, group, depends, request, var, s3_endpoint, metadata, tx)
