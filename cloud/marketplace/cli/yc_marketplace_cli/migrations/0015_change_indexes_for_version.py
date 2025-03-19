from yc_common import config
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.db.models import os_product_family_versions_table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback


def make_path(table_name):
    root = config.get_value("endpoints.kikimr.marketplace.root")
    return "{}/{}".format(root, table_name)


class Migration(BaseMigration):
    def execute(self) -> None:
        t_name = os_product_family_versions_table.name
        marketplace_db().copy_table(make_path(t_name), make_path(t_name + "_backup"))

    def rollback(self):
        raise MigrationNoRollback
