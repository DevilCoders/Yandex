# from cloud.marketplace.common.yc_marketplace_common.db.models import publisher_requests_table
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback


class Migration(BaseMigration):
    def execute(self) -> None:
        print("Outdated table")
        # publisher_requests_table.create()

    def rollback(self):
        raise MigrationNoRollback
