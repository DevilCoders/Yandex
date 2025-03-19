from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.db.models import tasks_table
from cloud.marketplace.common.yc_marketplace_common.lib import TaskUtils


class Migration(BaseMigration):
    def execute(self) -> None:
        TaskUtils.create(
            operation_type="export_tables",
            group_id=None,
            params={
                "timeout": 60 * 60 * 4,
                "export_destination": "logbroker",
            },
            is_infinite=True,
        )

    def rollback(self):
        with marketplace_db().with_table_scope(tasks_table).transaction() as tx:
            tx.query("DELETE FROM $table WHERE operation_type=?", "export_tables")
