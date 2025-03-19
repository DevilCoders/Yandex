from yc_common.clients.kikimr import KikimrDataType
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import os_product_families_table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback


class Migration(BaseMigration):
    def execute(self):
        os_product_families_table.add_column("billing_account_id", KikimrDataType.UTF8)

    def rollback(self):
        raise MigrationNoRollback()
