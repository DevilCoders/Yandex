from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTable
from yc_common.clients.kikimr import KikimrTableSpec
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import db_name
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table


class Migration(BaseMigration):
    def execute(self):
        KikimrTable(db_name, *DataModel(
            [Table(
                name="form",
                spec=KikimrTableSpec(
                    columns={
                        "id": KikimrDataType.UTF8,
                        "title": KikimrDataType.UTF8,
                        "billing_account_id": KikimrDataType.UTF8,
                        "public": KikimrDataType.BOOL,
                        "fields": KikimrDataType.JSON,
                        "schema": KikimrDataType.JSON,
                        "created_at": KikimrDataType.UINT64,
                        "updated_at": KikimrDataType.UINT64,
                    },
                    primary_keys=["id"],
                ),
            )],
        ).serialize_to_common()[0]).create()

    def rollback(self):
        # Never drop table in rollback (backup it!)
        pass
