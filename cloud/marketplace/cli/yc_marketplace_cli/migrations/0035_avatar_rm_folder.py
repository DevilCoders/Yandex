from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import avatars_table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback


class Migration(BaseMigration):
    def execute(self):
        avatars_table.drop_column("folder_id")

    def rollback(self):
        raise MigrationNoRollback()
