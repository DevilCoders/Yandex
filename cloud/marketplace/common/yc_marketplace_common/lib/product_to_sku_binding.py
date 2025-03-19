from yc_common.clients.kikimr.sql import SqlInsertValues

from cloud.marketplace.common.yc_marketplace_common.db.models import product_to_sku_binding_table
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction


class ProductToSkuBinding:
    @classmethod
    @mkt_transaction()
    def get_product_sku_id_set(cls, product_id, *, tx):
        result = tx.with_table_scope(product_to_sku_binding_table).select(
            "SELECT sku_id FROM $table WHERE product_id = ?",
            product_id, )

        return {i["sku_id"] for i in result}

    @classmethod
    @mkt_transaction()
    def update(cls, product_id, sku_ids, publisher_id, *, tx):
        if sku_ids is None:
            sku_ids = []

        tx_scoped = tx.with_table_scope(product_to_sku_binding_table)
        current_sku_id = cls.get_product_sku_id_set(product_id)
        to_add_sku_ids = set(sku_ids).difference(current_sku_id)
        to_delete_sku_ids = set(current_sku_id).difference(sku_ids)
        add_values = [(product_id, s_id, publisher_id) for s_id in to_add_sku_ids]
        if add_values:
            tx_scoped.query("UPSERT INTO $table ?",
                            SqlInsertValues(("product_id", "sku_id", "publisher_id"), add_values))
        delete_values = [(product_id, s_id) for s_id in to_delete_sku_ids]
        if delete_values:
            tx_scoped.query("DELETE FROM $table ON ?",
                            SqlInsertValues(("product_id", "sku_id"), delete_values))
