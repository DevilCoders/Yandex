from yc_common.clients.kikimr import KikimrDataType
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import publishers_table


class Migration(BaseMigration):
    def execute(self):
        publishers_table.add_column("billing_account_id", KikimrDataType.UTF8)

    def rollback(self):
        publishers_table.drop_column("billing_account_id")
