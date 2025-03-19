from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import migrations_table


class Migration(BaseMigration):
    def execute(self):
        migrations_table.drop(only_if_exists=True)
        migrations_table.create()

    def rollback(self):
        pass
