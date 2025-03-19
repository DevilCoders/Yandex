from typing import Optional

from grpc._cython import cygrpc

from yc_common.clients.kikimr import TransactionMode
from yc_common.clients.kikimr.sql import SqlNotIn
from yc_common.clients.kikimr.sql import SqlWhere
from yc_common.clients.models.operations import ErrorV1Beta1
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.misc import timestamp
from yc_common.paging import page_handler
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import partner_requests_table
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerTypes
from cloud.marketplace.common.yc_marketplace_common.models.partner_requests import PartnerRequest as PartnerRequestsScheme
from cloud.marketplace.common.yc_marketplace_common.models.partner_requests import PartnerRequestPublicCreate
from cloud.marketplace.common.yc_marketplace_common.models.partner_requests import PartnerRequestPublicList
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidPartnerRequestIdError
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidPartnerRequestTypeError
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction


class PartnerRequest:
    @staticmethod
    @mkt_transaction(tx_mode=TransactionMode.ONLINE_READ_ONLY_CONSISTENT)
    def get(id, *, tx):
        where = SqlWhere()
        where.and_condition("id = ?", id)
        where_query, where_args = where.render()
        request = tx.with_table_scope(partner_requests_table).select_one(
            "SELECT " + PartnerRequestsScheme.db_fields() +
            " FROM $table " + where_query, *where_args, model=PartnerRequestsScheme,
        )

        if request is None:
            raise InvalidPartnerRequestIdError()

        return request

    @staticmethod
    @mkt_transaction(tx_mode=TransactionMode.ONLINE_READ_ONLY_CONSISTENT)
    @page_handler(items="partner_requests")
    def rpc_list(cursor, limit=100, order_by: Optional[str] = None, filter: Optional[str] = None, tx=None):
        filter, filter_args = parse_filter(filter, PartnerRequestsScheme)

        if filter is not None:
            filter = " AND ".join(filter)

        where_query, where_args = page_query_args(
            cursor,
            limit,
            id="id",
            filter_query=filter,
            filter_args=filter_args,
            order_by=order_by,
        )

        iterator = tx.with_table_scope(partner_requests_table).select(
            "SELECT " + PartnerRequestsScheme.db_fields() + " " +
            "FROM $table " + where_query,
            *where_args, model=PartnerRequestsScheme,
        )

        requests_list = PartnerRequestPublicList({
            "partner_requests": [request.to_api(False) for request in iterator],
        })
        if limit is not None and len(requests_list.partner_requests) == limit:
            requests_list.next_page_token = requests_list.partner_requests[-1].id
        return requests_list

    @staticmethod
    @mkt_transaction()
    def create(request: PartnerRequestPublicCreate, *, tx):
        request = PartnerRequestsScheme.create(diff=request.diff, partner_id=request.partner_id, type=request.type)

        tx.with_table_scope(partner_requests_table).insert_object("INSERT INTO $table", request)

        return request

    @staticmethod
    @mkt_transaction()
    def fake_approved(request: PartnerRequestPublicCreate, *, tx):
        request = PartnerRequestsScheme.fake(PartnerRequestsScheme.Status.APPROVE,
                                             diff=request.diff, partner_id=request.partner_id, type=request.type)

        tx.with_table_scope(partner_requests_table).insert_object("INSERT INTO $table", request)

        return request

    @staticmethod
    @mkt_transaction()
    def rpc_approve(id, *, tx):
        partner_request = tx.with_table_scope(partner_requests_table).select_one(
            "SELECT " + PartnerRequestsScheme.db_fields() + " FROM $table WHERE id = ?", id,
            model=PartnerRequestsScheme)

        error = None

        if partner_request is None:
            error = ErrorV1Beta1({
                "code": cygrpc.StatusCode.not_found,
                "message": "request not found",
                "details": [],
            })
        elif partner_request.type == PartnerTypes.PUBLISHER:
            if not lib.publisher.Publisher.account_signed(partner_request.partner_id, tx=tx):
                error = ErrorV1Beta1({
                    "code": cygrpc.StatusCode.not_found,
                    "message": "Can't check account contract status in billing",
                    "details": [],
                })

        if error is None:
            tx.with_table_scope(partner_requests_table).update_object(
                "UPDATE $table $set WHERE ? AND id = ?", {"status": PartnerRequestsScheme.Status.APPROVE},
                SqlNotIn("status", PartnerRequestsScheme.Status.DONE), id,
            )

        op = OperationV1Beta1.new(
            id=generate_id(),
            description="approve_partner_update_request",
            created_at=timestamp(),
            created_by="",
            done=True,
            error=error,
            metadata=None,
            response=None,
        )

        return lib.TaskUtils.fake(op, tx=tx)

    @staticmethod
    @mkt_transaction()
    def rpc_decline(id, *, tx):
        tx.with_table_scope(partner_requests_table).update_object(
            "UPDATE $table $set WHERE ? AND id = ?", {"status": PartnerRequestsScheme.Status.DECLINE},
            SqlNotIn("status", PartnerRequestsScheme.Status.DONE), id,
        )

        op = OperationV1Beta1.new(
            id=generate_id(),
            description="decline_partner_update_request",
            created_at=timestamp(),
            created_by="",
            done=True,
            error=None,
            metadata=None,
            response=None,
        )

        return lib.TaskUtils.fake(op)

    @staticmethod
    @mkt_transaction()
    def rpc_close(request_id, auth_func, *, tx):
        request = tx.with_table_scope(partner_requests_table).select_one(
            "SELECT " + PartnerRequestsScheme.db_fields() + " FROM $table WHERE id = ?",
            request_id, model=PartnerRequestsScheme,
        )

        # check_auth
        if auth_func is not None:
            if request.type == PartnerTypes.PUBLISHER:
                lib.Publisher.rpc_get(request.partner_id, tx=tx, auth=auth_func)
            elif request.type == PartnerTypes.ISV:
                lib.Isv.rpc_get(request.partner_id, tx=tx, auth=auth_func)
            else:
                raise InvalidPartnerRequestTypeError()

        tx.with_table_scope(partner_requests_table).update_object(
            "UPDATE $table $set WHERE ? AND id = ?", {"status": PartnerRequestsScheme.Status.CLOSE},
            SqlNotIn("status", PartnerRequestsScheme.Status.DONE), request_id,
        )

        op = OperationV1Beta1.new(
            id=generate_id(),
            description="close_partner_update_request",
            created_at=timestamp(),
            created_by="",
            done=True,
            error=None,
            metadata=None,
            response=None,
        )

        return lib.TaskUtils.fake(op)
