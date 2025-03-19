from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTable
from yc_common.clients.kikimr import KikimrTableSpec
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import db_name
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback

data_model = DataModel((
    Table(name="os_product", spec=KikimrTableSpec(
        columns={
            "id": KikimrDataType.UTF8,
            "created_at": KikimrDataType.UINT64,
            "updated_at": KikimrDataType.UINT64,
            "folder_id": KikimrDataType.UTF8,
            "vendor": KikimrDataType.UTF8,
            "labels": KikimrDataType.JSON,
            "name": KikimrDataType.UTF8,
            "description": KikimrDataType.UTF8,
            "short_description": KikimrDataType.UTF8,
            "logo_id": KikimrDataType.UTF8,
            "logo_uri": KikimrDataType.UTF8,
            "status": KikimrDataType.UTF8,
            "primary_family_id": KikimrDataType.UTF8,
            "meta": KikimrDataType.JSON,
            "score": KikimrDataType.DOUBLE,
        },
        primary_keys=["id"],
    )),
))

os_products_table = KikimrTable(db_name, *(data_model.serialize_to_common()[0]))


class Migration(BaseMigration):
    def execute(self) -> None:
        os_products_table.add_column("meta", KikimrDataType.JSON)

    def rollback(self):
        raise MigrationNoRollback
