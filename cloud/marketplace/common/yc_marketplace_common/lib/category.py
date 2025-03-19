from typing import Optional

from yc_common.misc import drop_none
from yc_common.misc import timestamp
from yc_common.paging import page_handler
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import categories_table
from cloud.marketplace.common.yc_marketplace_common.lib.i18n import I18n
from cloud.marketplace.common.yc_marketplace_common.models.category import Category as CategoryScheme
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryList
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryMetadata
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryOperation
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidCategoryIdError
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction


class Category:
    @staticmethod
    @mkt_transaction()
    def rpc_get_noauth(category_id: str, *, tx) -> CategoryScheme:
        product = tx.with_table_scope(categories_table).select_one(
            "SELECT " + CategoryScheme.db_fields() + " " +
            "FROM $table "
            "WHERE id = ?", category_id,
            model=CategoryScheme)

        if product is None:
            raise InvalidCategoryIdError()

        return product

    @staticmethod
    @mkt_transaction()
    @page_handler(items="categories")
    def rpc_list(cursor: str,
                 limit: int,
                 order_by: Optional[str] = "score",  # TODO предполагается что у нас не очень много категорий
                 filter_query: Optional[str] = None,
                 tx=None,
                 ) -> CategoryList:

        filter_query, filter_args = parse_filter(filter_query, CategoryScheme)

        if filter_query is not None:
            filter_query = " AND ".join(filter_query)

        where_query, where_args = page_query_args(
            cursor,
            limit,
            id="id",
            filter_query=filter_query,
            filter_args=filter_args,
            order_by=order_by,
        )

        iterator = tx.with_table_scope(categories_table).select(
            "SELECT " + CategoryScheme.db_fields() + " " +
            "FROM $table " + where_query,
            *where_args, model=CategoryScheme)

        cl = CategoryList()
        for n, f in enumerate(iterator):
            if limit is not None and n == limit - 1:
                cl.next_page_token = f.id
            cl.categories.append(f.to_short())

        return cl

    @staticmethod
    @mkt_transaction()
    def rpc_create(request: CategoryCreateRequest, *, tx) -> CategoryOperation:
        text_dict = request.name
        category = CategoryScheme.from_request(request)
        category.name = I18n.set("category.{}.name".format(category.id), text_dict)

        tx.with_table_scope(categories_table).insert_object("INSERT INTO $table", category)
        op = CategoryOperation({
            "id": category.id,
            "description": "category_create",
            "created_at": timestamp(),
            "done": True,
            "metadata": CategoryMetadata({"category_id": category.id}).to_primitive(),
            "response": category.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_category_create", tx=tx)

    @staticmethod
    @mkt_transaction()
    def rpc_update(request: CategoryUpdateRequest, *, tx) -> CategoryOperation:

        category = Category.rpc_get_noauth(request.id, tx=tx)

        category_update = drop_none({
            "name": request.name,
            "type": request.type,
            "score": request.score,
            "parent_id": request.parent_id,
        })
        if category_update:
            if "name" in category_update:
                category_update["name"] = I18n.set("category.{}.name".format(category.id), category_update["name"])

            for k in category_update:
                setattr(category, k, category_update[k])
            tx.with_table_scope(categories_table).update_object("UPDATE $table $set WHERE id = ?", category_update,
                                                                request.os_product_id)

        op = CategoryOperation({
            "id": category.id,
            "description": "category_update",
            "created_at": timestamp(),
            "done": True,
            "metadata": CategoryMetadata({"category_id": category.id}).to_primitive(),
            "response": category.to_public().to_api(True),
        })

        return lib.TaskUtils.fake(op, "fake_category_update", tx=tx)
