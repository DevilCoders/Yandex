from typing import Iterable
from typing import Optional
from typing import Tuple
from typing import Union

from yc_common import config
from yc_common import logging
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.clients.kikimr.sql import SqlInsertValues
from yc_common.clients.kikimr.sql import SqlNotIn
from yc_common.misc import timestamp
from yc_common.paging import read_page
from yc_common.validation import ResourceIdType
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import ordering_table
from cloud.marketplace.common.yc_marketplace_common.lib import Category
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryMetadata
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryOperation
from cloud.marketplace.common.yc_marketplace_common.models.ordering import OrderingList
from cloud.marketplace.common.yc_marketplace_common.models.ordering import OrderingShortView
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction

log = logging.get_logger(__name__)


class Ordering:
    @staticmethod
    @mkt_transaction()
    def set_order(resource_id: ResourceIdType,
                  categories: Optional[Iterable[Union[Tuple[str, int], str]]] = None, *, tx) -> None:

        if categories is None:
            categories = []
        step = config.get_value("marketplace.ordering_step", default=10)

        order_lookup = {}
        category_ids = set()

        # Set new input to canonical view
        # TODO когда определимся с типом наверное для переданного дикта стоит обновлять не только новые категории,
        # а все переданные
        need_maximum = []
        for c in categories:
            if isinstance(c, tuple):
                order_lookup[c[0]] = c[1]
                category_ids.add(c[0])
            else:
                order_lookup[c] = step
                need_maximum.append(c)
                category_ids.add(c)

        tx = tx.with_table_scope(ordering_table)

        if len(need_maximum) != 0:  # Get maximum value of category ordering not in current product for add to the end
            for x in tx.select("SELECT MAX(`order`) as o, category_id FROM $table "
                               "WHERE ? AND resource_id != ? GROUP BY category_id",
                               SqlIn("category_id", need_maximum), resource_id):
                order_lookup[x["category_id"]] = x["o"] + step

        current_cat_ids_select = tx.select("SELECT category_id FROM $table "
                                           "WHERE ? AND resource_id = ? GROUP BY category_id",
                                           SqlIn("category_id", category_ids), resource_id)

        current_cat_ids = {x["category_id"] for x in current_cat_ids_select}

        new_category_ids = category_ids - current_cat_ids
        # remove categories that are not listed
        tx.query("DELETE FROM $table WHERE resource_id = ? AND ? ", resource_id, SqlNotIn("category_id", category_ids))

        # update order for new sended categories
        for cid in new_category_ids:
            update = {
                "resource_id": resource_id,
                "category_id": cid,
                "order": order_lookup.get(cid),
            }
            log.debug("update: {}".format(update))
            tx.insert_object("REPLACE INTO $table", update)

    @staticmethod
    @mkt_transaction()
    def move_to_position(resource_id: ResourceIdType, category_id: ResourceIdType, pos: int, *, tx) -> None:
        """
        Put resource on place
        :param resource_id: Resource Id
        :param category_id: Category Id
        :param pos: Position in category. Zero based
        """
        step = config.get_value("marketplace.ordering_step", default=10)

        tx = tx.with_table_scope(ordering_table)
        resources = tx.select(
            "SELECT resource_id, `order` FROM $table "
            "WHERE category_id = ? AND resource_id != ? "
            "ORDER BY `order`", category_id, resource_id)
        count = len(resources)
        if pos > count:
            pos = count
        start = resources[pos - 1].get("order") if pos > 0 else 0
        end = resources[pos].get("order") if pos < count else resources[count - 1].get("order") + step

        if end - start > 1:
            mods = [(resource_id, (start + end) // 2)]
        else:
            i = pos
            order = start + 1
            mods = [(resource_id, order)]
            while True:
                order += 1
                mods.append((resources[i].get("resource_id"), order))
                if resources[i + 1].get("order") != order:
                    break
                i += 1
            # if we have a lot of order collisions (> count//2), it's time to reorder all items
            if len(mods) > count // 2:
                new_order = resources[:pos] + [{"resource_id": resource_id}] + resources[pos:]
                mods = [(r, (j + 1) * step) for j, r in enumerate([el.get("resource_id") for el, _ in new_order])]
        log.debug("update: {}".format(mods))
        sql_values = [(category_id, mod[0], mod[1]) for mod in mods]
        tx.query("UPSERT INTO $table ?", SqlInsertValues(("category_id", "resource_id", "order"), sql_values))

    @staticmethod
    @mkt_transaction()
    def _read(cursor: Optional[str],
              limit: Optional[int],
              category_id: ResourceIdType,
              *, tx, **kwargs) -> OrderingList:
        where_query, where_args = page_query_args(
            cursor,
            limit,
            id="resource_id",
            filter_query="category_id = ?",
            filter_args=[category_id],
            order_by="order DESC",
        )
        iterator = tx.with_table_scope(ordering_table).select(
            "SELECT " + OrderingShortView.db_fields() + " " +
            "FROM $table " + where_query,
            *where_args, model=OrderingShortView)

        resources = OrderingList({
            "resource_orders": iterator,
        })

        if limit is not None and len(resources.resource_orders) == limit:
            resources.next_page_token = resources.resource_orders[-1].resource_id

        return resources

    @classmethod
    @mkt_transaction()
    def get_ids(cls, category_id: Optional[str] = None,
                page_size=100,
                page_token: Optional[str] = None,
                *,
                tx) -> OrderingList:
        return read_page(cls._read, page_token, page_size, "resource_orders", category_id=category_id, tx=tx)

    @classmethod
    @mkt_transaction()
    def nullify_priority(cls, category_id: str, resources_list: list, *, tx):
        tx.with_table_scope(ordering_table).update_object("REPLACE $table $set WHERE ? and category_id = ?",
                                                          {"order": 0},
                                                          SqlIn("resource_id", resources_list),
                                                          category_id)

    @classmethod
    @mkt_transaction()
    def update_priority(cls, category_id: str, priority_list: list, *, tx):
        count_resources = len(priority_list)
        step = config.get_value("marketplace.ordering_step", default=10)
        sql_values = [(category_id, priority_list[index], index * step) for index in
                      range(count_resources)]
        tx.with_table_scope(ordering_table).query("UPSERT INTO $table ?",
                                                  SqlInsertValues(("category_id", "resource_id", "order"), sql_values))

        category = Category.rpc_get_noauth(category_id, tx=tx)
        op = CategoryOperation({
            "id": category.id,
            "description": "add_to_category",
            "created_at": timestamp(),
            "done": True,
            "metadata": CategoryMetadata({"category_id": category.id}).to_primitive(),
            "response": category.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_add_to_category", tx=tx)

    @classmethod
    @mkt_transaction()
    def remove(cls, category_id: str, resource_list: list, *, tx):
        sql_values = [(category_id, r) for r in resource_list]
        tx.with_table_scope(ordering_table).query("DELETE FROM $table ON ?",
                                                  SqlInsertValues(("category_id", "resource_id"), sql_values))

        category = Category.rpc_get_noauth(category_id, tx=tx)
        op = CategoryOperation({
            "id": category.id,
            "description": "remove_from_category",
            "created_at": timestamp(),
            "done": True,
            "metadata": CategoryMetadata({"category_id": category.id}).to_primitive(),
            "response": category.to_public().to_api(True),
        })
        return lib.TaskUtils.fake(op, "fake_remove_from_category", tx=tx)
