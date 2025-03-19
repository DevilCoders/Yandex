from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import var_table


class Migration(BaseMigration):
    def execute(self):
        var_table.create()

    def rollback(self):
        var_table.drop()
