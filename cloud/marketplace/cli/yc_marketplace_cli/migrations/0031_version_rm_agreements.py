from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import os_product_family_versions_table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback


class Migration(BaseMigration):
    def execute(self):
        os_product_family_versions_table.drop_column("agreement_ids")

    def rollback(self):
        raise MigrationNoRollback()
