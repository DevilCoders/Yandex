from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import tasks_table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback


class Migration(BaseMigration):
    def execute(self) -> None:
        tasks_table.client.query("UPDATE $table SET group_id=id WHERE group_id is NULL", commit=True)
        tasks_table.client.query("UPDATE $table SET kind='internal' WHERE kind is NULL", commit=True)

    def rollback(self):
        raise MigrationNoRollback
