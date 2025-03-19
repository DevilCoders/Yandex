from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import product_slugs_table


class Migration(BaseMigration):
    def execute(self):
        product_slugs_table.create()

    def rollback(self):
        # Never drop table in rollback (backup it!)
        pass
