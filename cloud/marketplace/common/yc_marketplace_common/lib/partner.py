from abc import ABC
from abc import abstractmethod
from typing import Optional
from typing import Tuple
from typing import Type

import grpc
from grpc._common import STATUS_CODE_TO_CYGRPC_STATUS_CODE

from yc_common import config
from yc_common import logging
from yc_common.clients.kikimr import ColumnStrippingStrategy
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.clients.kikimr.sql import SqlWhere
from yc_common.clients.models.operations import ErrorV1Beta1
from yc_common.misc import drop_none
from yc_common.misc import timestamp
from yc_common.validation import ResourceIdType
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import categories_table
from cloud.marketplace.common.yc_marketplace_common.db.models import ordering_table
from cloud.marketplace.common.yc_marketplace_common.lib.i18n import I18n
from cloud.marketplace.common.yc_marketplace_common.models.partner import PartnerInterface
from cloud.marketplace.common.yc_marketplace_common.models.partner_requests import PartnerRequestPublicCreate
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter_with_category
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_order_by
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args_with_complex_cursor
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction

log = logging.get_logger(__name__)


def fake_operation(cls, cls_op, obj, error, tx, **kwargs):
    if error is not None:
        resp = None
    elif obj is not None:
        resp = obj.to_api(True)
    else:
        resp = cls.rpc_get(id).to_api(True)

    op = cls_op(
        id=id,
        created_at=timestamp(),
        created_by="",
        done=True,
        error=error,
        response=resp,
        **kwargs,
    )

    return lib.TaskUtils.fake(op, tx=tx)


class Partner(ABC):
    @property
    @abstractmethod
    def default_category(self):
        pass

    @property
    @abstractmethod
    def table(self):
        pass

    @property
    @abstractmethod
    def model(self) -> Type[PartnerInterface]:
        pass

    @property
    @abstractmethod
    def response_model(self):
        pass

    @property
    @abstractmethod
    def partner_type(self):
        pass

    @property
    @abstractmethod
    def category_type(self):
        pass

    @property
    @abstractmethod
    def error_not_found(self) -> Type[Exception]:
        pass

    @property
    @abstractmethod
    def error_conflict(self) -> Type[Exception]:
        pass

    @classmethod
    @mkt_transaction()
    def _get(cls, query_arg, *, tx, auth=None):
        where = SqlWhere()
        for query, arg in query_arg:
            where.and_condition(query, arg)
        where_query, where_args = where.render()
        partner = tx.with_table_scope(cls.table).select_one(
            "SELECT " + cls.model.db_fields() + " FROM $table " + where_query,
            *where_args, model=cls.model, ignore_unknown=True,
        )

        if partner is None:
            raise cls.error_not_found()

        if auth is not None:
            auth(partner)

        categories = tx.with_table_scope(ordering_table) \
            .select_one("SELECT LIST(category_id) as ids FROM $table WHERE resource_id = ?", partner.id)

        data = partner.to_primitive()

        if categories is not None:
            data["categories"] = categories["ids"]

        return partner.PublicModel(data)

    @classmethod
    @mkt_transaction()
    def rpc_get(cls, partner_id, *, tx, auth=None):
        return cls._get([["id = ?", partner_id]], tx=tx, auth=auth)

    @classmethod
    @mkt_transaction()
    def rpc_get_by_ba(cls, ba_id, *, tx, auth=None):
        return cls._get([["billing_account_id = ?", ba_id]], tx=tx, auth=auth)

    @classmethod
    @mkt_transaction()
    def rpc_get_public(cls, partner_id, *, tx):
        return cls._get([["id = ?", partner_id], ["status = ?", cls.model.Status.ACTIVE]], tx=tx)

    @classmethod
    def _rpc_list(cls, cursor, limit, *, filter_query=None, order_by=None, active=False, billing_account_id=None,
                  tx=None):
        where_query, where_args = cls._prepare_list_sql_conditions(
            cursor, limit, filter_query, order_by, active=active, billing_account_id=billing_account_id, tx=tx,
        )

        query = """
            SELECT * FROM $partner_table as partner
            JOIN $ordering_table as ordering
            ON partner.id = ordering.resource_id
        """

        result = tx.with_table_scope(partner=cls.table, ordering=ordering_table).select(
            query + where_query, *where_args,
            model=cls.model, ignore_unknown=True,
        )

        return cls._enrich_list(result, tx), limit

    @classmethod
    def _enrich_list(cls, select, tx):
        resp = []
        ids = [row.id for row in select]

        categories = {}
        query = """
            SELECT LIST(ordering.category_id) as ids, ordering.resource_id as resource_id
            FROM $ordering_table as ordering
            WHERE ?
            GROUP BY ordering.resource_id
        """

        categories_raw = tx.with_table_scope(category=categories_table, ordering=ordering_table). \
            select(query, SqlIn("ordering.resource_id", ids))

        for row in categories_raw:
            categories[row["resource_id"]] = row["ids"]

        for row in select:
            data = row.to_primitive()
            data["categories"] = categories.get(row.id, [])
            resp.append(cls.response_model(data))

        return resp

    @classmethod
    @mkt_transaction()
    def _get_with_order(cls, partner_id: ResourceIdType, category_id: ResourceIdType, *, tx):
        partner = tx.with_table_scope(partner=cls.model, order=ordering_table).select_one(
            "SELECT o.`order`, p.* "
            "FROM $partner_table as p "
            "JOIN $order_table as o "
            "ON o.resource_id = p.id "
            "WHERE p.id = ? AND o.category_id = ? ", partner_id, category_id,
            model=cls.model, ignore_unknown=True,
            strip_table_from_columns=ColumnStrippingStrategy.STRIP_AND_MERGE)

        if partner is None:
            raise cls.error_not_found()

        return partner

    @classmethod
    def _prepare_list_sql_conditions(cls,
                                     cursor: Optional[ResourceIdType],
                                     limit: Optional[int],
                                     filter_query: Optional[str],
                                     order_by: Optional[str],
                                     active=True,
                                     billing_account_id=None,
                                     tx=None) -> Tuple[str, list]:
        cursor_partner = {}
        filter_query_list, filter_args, category_id = parse_filter_with_category(filter_query, PartnerInterface)
        mapping = {
            "order": "ordering.`order`",
            "id": "partner.id",
            "createdAt": "partner.created_at",
            "name": "partner.name",
            "description": "partner.description",
            "status": "partner.status",
            "billingAccountId": "partner.billing_account_id",
        }

        order_by = parse_order_by(order_by, mapping, "order")
        if category_id is None:
            category_id = config.get_value(cls.default_category)

        filter_query_list += ["ordering.category_id = ?"]
        filter_args += [category_id]

        if cursor:
            cursor_partner = cls._get_with_order(cursor, category_id, tx=tx)

        if billing_account_id is not None:
            filter_query_list += ["partner.billing_account_id = ?"]
            filter_args += [billing_account_id]

        if active:
            filter_query_list += ["partner.status = ?"]
            filter_args += [cls.model.Status.ACTIVE]

        filter_query = " AND ".join(filter_query_list)

        where_query, where_args = page_query_args_with_complex_cursor(
            cursor,
            cursor_partner,
            limit,
            mapping,
            id="id",
            filter_query=filter_query,
            filter_args=filter_args,
            order_by=order_by,
        )
        return where_query, where_args

    @classmethod
    def _rpc_create(cls, request, tx, admin=False):
        meta = request.meta
        request.meta = {}
        partner = cls.model.from_request(request)

        if tx.with_table_scope(cls.table).select_one("SELECT id FROM $table WHERE billing_account_id=?",
                                                     partner.billing_account_id):
            raise cls.error_conflict()
        create_request = PartnerRequestPublicCreate({
            "diff": {
                "status": cls.model.Status.ACTIVE,
            },
            "partner_id": partner.id,
            "type": cls.partner_type,
        })

        if admin:
            req = lib.PartnerRequest.fake_approved(create_request)
        else:
            req = lib.PartnerRequest.create(create_request)

        for key in meta:
            partner.meta[key] = I18n.set("partner.{}.meta.{}".format(partner.id, key), meta[key])

        tx.with_table_scope(cls.table).insert_object("INSERT INTO $table", partner)
        tx.with_table_scope(ordering_table).insert_object("UPSERT INTO $table", {
            "category_id": config.get_value(cls.default_category),
            "resource_id": partner.id,
            "order": 1,
        })

        return req, partner

    @classmethod
    def _rpc_update(cls, request, id, op, metadata, tx, auth):
        meta = request.meta
        update = drop_none({
            "name": request.name,
            "description": request.description,
            "logo_id": request.logo_id,
            "contact_info": request.contact_info,
            "meta": {},
        })
        for key in meta:
            update["meta"][key] = I18n.set("partner.{}.meta.{}".format(id, key), meta[key])
        partner = cls.rpc_get(id, tx=tx, auth=auth)

        if partner.status == cls.model.Status.ERROR:
            return fake_operation(
                cls, op, None, ErrorV1Beta1({
                    "code": STATUS_CODE_TO_CYGRPC_STATUS_CODE[grpc.StatusCode.INTERNAL],
                    "message": "Can not update partner in Error status",
                }), id=id, description="update_partner", tx=tx,
                metadata=metadata,  #
            )

        return lib.PartnerRequest.create(PartnerRequestPublicCreate({
            "diff": update,
            "partner_id": partner.id,
            "type": cls.partner_type,
        })), partner

    @classmethod
    @mkt_transaction()
    def update(cls, id, update, *, tx):
        tx.with_table_scope(cls.table).update_object(
            "UPDATE $table $set WHERE id = ? AND status != ?",
            update, id, cls.model.Status.ERROR,
        )

    @classmethod
    def _task_update_start(cls, request, tx):
        group = generate_id()

        return lib.TaskUtils.create(
            operation_type="check_partner_request",
            group_id=group,
            params={"id": request.id},
            metadata={},
            tx=tx,
        ), group

    @classmethod
    def _task_update_finish(cls, start_task, group, depends, request, partner, s3_endpoint, metadata, tx):
        if "logo_id" in request.diff:
            lib.Avatar.capture_image(request.diff["logo_id"], partner.id, tx=tx)
            publish_logo = lib.Avatar.task_publish(
                request.diff["logo_id"],
                partner.id,
                s3_endpoint,
                group_id=group,
                rewrite=True,
                depends=[start_task.id],
                tx=tx,
            )
            depends.append(publish_logo.id)

        return lib.TaskUtils.create(
            operation_type="finalize_partner",
            group_id=group,
            params={
                "id": partner.id,
                "diff": request.diff,
                "type": cls.partner_type,
            },
            metadata=metadata,
            depends=depends,
            tx=tx,
        )
