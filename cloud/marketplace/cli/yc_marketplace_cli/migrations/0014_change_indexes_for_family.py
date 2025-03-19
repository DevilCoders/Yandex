from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.db.models import os_product_families_table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback


class Migration(BaseMigration):
    def execute(self) -> None:
        os_product_families_table.drop()
        os_product_families_table.create()
        with marketplace_db().transaction() as tx:
            data = list(tx.select("SELECT * FROM {}".format(os_product_families_table.name + "_backup")))
            for row in data:
                tx.with_table_scope(os_product_families_table).insert_object("UPSERT INTO $table", row)

    def rollback(self):
        raise MigrationNoRollback
