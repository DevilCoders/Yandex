from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTable
from yc_common.clients.kikimr import KikimrTableSpec
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table

isv_table = KikimrTable("marketplace",
                        *(DataModel(
                            (
                                Table(
                                    name="isv", spec=KikimrTableSpec(
                                        columns={
                                            "id": KikimrDataType.UTF8,
                                            "billing_account_id": KikimrDataType.UTF8,
                                            "incentive": KikimrDataType.JSON,
                                            "folder_id": KikimrDataType.UTF8,
                                            "name": KikimrDataType.UTF8,
                                            "description": KikimrDataType.UTF8,
                                            "contact_info": KikimrDataType.JSON,
                                            "logo_id": KikimrDataType.UTF8,
                                            "logo_uri": KikimrDataType.UTF8,
                                            "status": KikimrDataType.UTF8,
                                            "created_at": KikimrDataType.UINT64,
                                            "meta": KikimrDataType.JSON,
                                        },
                                        primary_keys=["id"],
                                    ),
                                ),
                            )).serialize_to_common()[0]))


class Migration(BaseMigration):
    def execute(self) -> None:
        isv_table.drop_column("incentive", True)

    def rollback(self):
        isv_table.add_column("incentive", KikimrDataType.JSON)
