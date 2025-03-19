from typing import Optional

from yc_common import config
from yc_common import logging
from yc_common.misc import timestamp
from yc_common.paging import page_handler

from cloud.marketplace.common.yc_marketplace_common.client import billing
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import sku_draft_table
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import CreateSkuDraftRequest
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraft as SkuDraftScheme
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftList
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftMetadata
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftOperation
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftStatus
from cloud.marketplace.common.yc_marketplace_common.utils.errors import SkuDraftNameConflictError
from cloud.marketplace.common.yc_marketplace_common.utils.errors import SkuDraftNotFoundError
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_order_by
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction

log = logging.get_logger('yc_marketplace_common.models.sku_draft')


class SkuDraft:
    @classmethod
    @mkt_transaction()
    def get(cls, id, *, tx, auth=None) -> SkuDraftScheme:
        draft = tx.with_table_scope(sku_draft_table) \
            .select_one("SELECT {} FROM $table WHERE id = ?".format(SkuDraftScheme.db_fields()), id,
                        model=SkuDraftScheme)

        if draft is None:
            raise SkuDraftNotFoundError()

        if auth is not None:
            auth(draft)

        return draft

    @classmethod
    @mkt_transaction()
    def rpc_get(cls, id, *, tx, auth=None):
        return cls.get(id, tx=tx, auth=auth).to_public()

    @staticmethod
    @mkt_transaction()
    @page_handler(items="sku_drafts")
    def rpc_list(cursor: Optional[str],
                 limit: Optional[int] = 100,
                 *,
                 billing_account_id=None,
                 order_by: Optional[str] = None,
                 filter_query: Optional[str] = None,
                 tx=None) -> SkuDraftList:
        filter_query, filter_args = parse_filter(filter_query, SkuDraftScheme)

        if billing_account_id:
            filter_query += ["billing_account_id = ?"]
            filter_args += [billing_account_id]

        filter_query = " AND ".join(filter_query)

        mapping = {
            "id": "id",
            "billingAccountId": "billing_account_id",
            "name": "name",
            "status": "status",
        }

        if order_by:
            order_by = parse_order_by(order_by, mapping, "id")

        where_query, where_args = page_query_args(
            cursor,
            limit,
            id="id",
            filter_query=filter_query,
            filter_args=filter_args,
            order_by=order_by,
        )

        query = "SELECT " + SkuDraftScheme.db_fields() + " FROM $table " + where_query

        log.info("%s %s", query, where_args)
        rows = tx.with_table_scope(sku_draft_table).select(query, *where_args, model=SkuDraftScheme)

        response = SkuDraftList()

        for draft in rows:
            # We assume that we have something else if we have reached the limit

            response.sku_drafts.append(draft.to_public())

        if limit is not None and len(response.sku_drafts) == limit:
            response.next_page_token = response.sku_drafts[-1].id

        return response

    @classmethod
    @mkt_transaction()
    def rpc_create(cls, request: CreateSkuDraftRequest, *, tx):
        draft = request.to_model()
        draft.validate()

        uniq_check = tx.with_table_scope(sku_draft_table).select_one(
            "SELECT " + SkuDraftScheme.db_fields() + " " +
            "FROM $table "
            "WHERE billing_account_id = ? "
            "AND publisher_account_id = ? "
            "AND name = ? ", draft.billing_account_id, draft.publisher_account_id, draft.name,
            model=SkuDraftScheme)

        if uniq_check:
            raise SkuDraftNameConflictError()

        tx.with_table_scope(sku_draft_table).insert_object("INSERT INTO $table", draft)
        op = SkuDraftOperation({
            "id": draft.id,
            "description": "sku_draft_create",
            "created_at": timestamp(),
            "done": True,
            "metadata": SkuDraftMetadata({"sku_draft_id": draft.id}).to_primitive(),
            "response": draft.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_sku_draft_create", tx=tx)

    @classmethod
    @mkt_transaction()
    def rpc_accept(cls, sku_draft_id, *, tx, auth=None):
        draft = cls.get(sku_draft_id, tx=tx, auth=auth)
        publisher = lib.Publisher.rpc_get(draft.publisher_account_id)
        sku_name = "marketplace.{}.{}".format(draft.publisher_account_id, draft.name)
        service_id = config.get_value("marketplace.service_id")
        balance_product_id = config.get_value("marketplace.balance_product_id")

        log.info("SKU DRAFT: %s", draft)
        log.info("SKU DRAFT: %s", draft.to_sku_pricing_versions(draft.created_at))

        req = billing.CreateSkuRequest({
            "name": sku_name,
            "service_id": service_id,
            "balance_product_id": balance_product_id,
            "metric_unit": draft.metric_unit,
            "usage_unit": draft.usage_unit,
            "pricing_unit": draft.pricing_unit,
            "pricing_versions": draft.to_sku_pricing_versions(draft.created_at),
            "formula": draft.formula,
            "publisher_account_id": publisher.billing_publisher_account_id,
            "rate_formula": draft.rate_formula,
            "resolving_policy": draft.resolving_policy,
            "translations": draft.meta,
            "schemas": draft.schemas,
            "is_unique_schema": False,
        })
        lib.Billing.create_sku(config.get_value("endpoints.billing.url"), request=req)
        tx.with_table_scope(sku_draft_table) \
            .query("UPDATE $table SET status=? WHERE id = ?", SkuDraftStatus.ACTIVE, draft.id)
        draft.status = SkuDraftStatus.ACTIVE
        op = SkuDraftOperation({
            "id": generate_id(),
            "description": "sku_draft_accept",
            "created_at": timestamp(),
            "done": True,
            "metadata": SkuDraftMetadata({"sku_draft_id": draft.id}).to_primitive(),
            "response": draft.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_sku_draft_accept", tx=tx)

    @classmethod
    @mkt_transaction()
    def rpc_reject(cls, sku_draft_id, *, tx, auth=None):
        draft = cls.get(sku_draft_id, tx=tx, auth=auth)
        tx.with_table_scope(sku_draft_table) \
            .query("UPDATE $table SET status=? WHERE id = ?", SkuDraftStatus.REJECTED, draft.id)
        draft.status = SkuDraftStatus.REJECTED
        op = SkuDraftOperation({
            "id": generate_id(),
            "description": "sku_draft_reject",
            "created_at": timestamp(),
            "done": True,
            "metadata": SkuDraftMetadata({"sku_draft_id": draft.id}).to_primitive(),
            "response": draft.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_sku_draft_reject", tx=tx)
