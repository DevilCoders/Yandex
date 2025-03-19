from typing import Optional

from yc_common.misc import timestamp
from yc_common.paging import page_handler
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import forms_table
from cloud.marketplace.common.yc_marketplace_common.models.form import FormCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.form import FormList
from cloud.marketplace.common.yc_marketplace_common.models.form import FormMetadata
from cloud.marketplace.common.yc_marketplace_common.models.form import FormOperation
from cloud.marketplace.common.yc_marketplace_common.models.form import FormScheme
from cloud.marketplace.common.yc_marketplace_common.models.form import FormUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.utils.errors import FormNotFoundError
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_order_by
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction


class Form:
    @classmethod
    def get(cls, id, *, tx, auth=None):
        form = tx.with_table_scope(forms_table)\
            .select_one("SELECT {} FROM $table WHERE id = ?".format(FormScheme.db_fields()), id, model=FormScheme)

        if form is None:
            raise FormNotFoundError()

        if auth is not None:
            auth(form)

        return form

    @classmethod
    @mkt_transaction()
    def rpc_get(cls, id, *, tx, auth=None):
        return cls.get(id, tx=tx, auth=auth).to_public()

    @staticmethod
    @mkt_transaction()
    @page_handler(items="forms")
    def rpc_list(cursor: Optional[str],
                 limit: Optional[int] = 100,
                 *,
                 order_by: Optional[str] = None,
                 filter_query: Optional[str] = None,
                 tx=None) -> FormList:
        filter_query, filter_args = parse_filter(filter_query, FormScheme)

        filter_query = " AND ".join(filter_query)

        mapping = {
            "id": "id",
            "billing_account_id": "billingAccountId",
            "public": "public",
            "title": "title",
        }

        order_by = parse_order_by(order_by, mapping, "id")

        where_query, where_args = page_query_args(
            cursor,
            limit,
            id="id",
            filter_query=filter_query,
            filter_args=filter_args,
            order_by=order_by,
        )

        rows = tx.with_table_scope(forms_table).select(
            "SELECT " + FormScheme.db_fields() +
            " FROM $table " + where_query,
            *where_args, model=FormScheme)

        response = FormList()

        for form in rows:
            # We assume that we have something else if we have reached the limit

            response.forms.append(form.to_public())

        if limit is not None and len(response.forms) == limit:
            response.next_page_token = response.forms[-1].id

        return response

    @classmethod
    @mkt_transaction()
    def rpc_create(cls, request: FormCreateRequest, *, tx):
        form = FormScheme.from_create_request(request)

        tx.with_table_scope(forms_table).insert_object("INSERT INTO $table", form)
        op = FormOperation({
            "id": form.id,
            "description": "form_create",
            "created_at": timestamp(),
            "done": True,
            "metadata": FormMetadata({"form_id": form.id}).to_primitive(),
            "response": form.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_form_create", tx=tx)

    @classmethod
    @mkt_transaction()
    def rpc_update(cls, request: FormUpdateRequest, *, tx, auth=None):
        form = cls.get(request.id, tx=tx, auth=auth)

        if request.schema:
            form.schema = request.schema

        if request.fields:
            form.fields = request.fields

        if request.title:
            form.title = request.title

        tx.with_table_scope(forms_table).insert_object("UPSERT INTO $table", form)
        op = FormOperation({
            "id": form.id,
            "description": "form_update",
            "created_at": timestamp(),
            "done": True,
            "metadata": FormMetadata({"form_id": form.id}).to_primitive(),
            "response": form.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_form_update", tx=tx)
