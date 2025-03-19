from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTable
from yc_common.clients.kikimr import KikimrTableSpec
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import db_name
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback

data_model = DataModel((
    Table(name="os_product_family", spec=KikimrTableSpec(
        columns={
            "id": KikimrDataType.UTF8,
            "folder_id": KikimrDataType.UTF8,
            "created_at": KikimrDataType.UINT64,
            "updated_at": KikimrDataType.UINT64,
            "labels": KikimrDataType.JSON,
            "name": KikimrDataType.UTF8,
            "description": KikimrDataType.UTF8,
            "status": KikimrDataType.UTF8,
            "os_product_id": KikimrDataType.UTF8,
            "deprecation": KikimrDataType.JSON,
            "meta": KikimrDataType.JSON,
        },
        primary_keys=["id"],
    )),
))

os_product_families_table = KikimrTable(db_name, *(data_model.serialize_to_common()[0]))


class Migration(BaseMigration):
    def execute(self) -> None:
        os_product_families_table.add_column("meta", KikimrDataType.JSON)

    def rollback(self):
        raise MigrationNoRollback
