from yc_common import config
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import categories_table
from cloud.marketplace.common.yc_marketplace_common.db.models import isv_table
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.db.models import ordering_table
from cloud.marketplace.common.yc_marketplace_common.db.models import publishers_table
from cloud.marketplace.common.yc_marketplace_common.db.models import var_table
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback


class Migration(BaseMigration):
    def execute(self):
        with marketplace_db().transaction() as tx:
            data = {
                "publisher": publishers_table,
                "var": var_table,
                "isv": isv_table,
            }
            category_types = {
                "publisher": Category.Type.PUBLISHER,
                "var": Category.Type.VAR,
                "isv": Category.Type.ISV,
            }

            for partner_type in data:
                category_id = config.get_value("marketplace.default_{}_category".format(partner_type))
                category = Category.new("All {}s".format(partner_type), category_types[partner_type], 0, None)
                category.id = category_id
                tx.with_table_scope(categories_table).insert_object("UPSERT INTO $table", category)

                resources = tx.with_table_scope(data[partner_type]).select("SELECT id FROM $table")
                if resources is None:
                    continue

                ordering_tx = tx.with_table_scope(ordering_table)

                for obj in resources:
                    ordering = ordering_tx.select_one(
                        "SELECT * FROM $table WHERE category_id = ? AND resource_id = ?", category_id, obj["id"])

                    if ordering is None:
                        ordering_tx.insert_object("UPSERT INTO $table", {
                            "category_id": category_id,
                            "resource_id": obj["id"],
                            "order": 1,
                        })

    def rollback(self):
        raise MigrationNoRollback
