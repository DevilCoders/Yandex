from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import isv_table


class Migration(BaseMigration):
    def execute(self) -> None:
        isv_table.create()

    def rollback(self):
        isv_table.drop()
