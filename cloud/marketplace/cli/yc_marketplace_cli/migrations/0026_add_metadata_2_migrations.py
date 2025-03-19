from yc_common.clients.kikimr import KikimrDataType
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import migrations_table


class Migration(BaseMigration):
    def execute(self):
        migrations_table.add_column("metadata", KikimrDataType.JSON)

    def rollback(self):
        print("Nothing")
