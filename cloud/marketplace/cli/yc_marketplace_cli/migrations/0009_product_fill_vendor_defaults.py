from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTable
from yc_common.clients.kikimr import KikimrTableSpec
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import db_name
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback

products_data_model = DataModel((
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
            "score": KikimrDataType.DOUBLE,
        },
        primary_keys=["id"],
    )),
))

os_products_table = KikimrTable(db_name, *(products_data_model.serialize_to_common()[0]))

publishers_data_model = DataModel((
    Table(name="publishers", spec=KikimrTableSpec(
        columns={
            "id": KikimrDataType.UTF8,
            "billing_publisher_account_id": KikimrDataType.UTF8,
            "folder_id": KikimrDataType.UTF8,
            "name": KikimrDataType.UTF8,
            "description": KikimrDataType.UTF8,
            "contact_info": KikimrDataType.JSON,
            "logo_id": KikimrDataType.UTF8,
            "logo_uri": KikimrDataType.UTF8,
            "status": KikimrDataType.UTF8,
            "created_at": KikimrDataType.UINT64,
        },
        primary_keys=["id"],
    )),
))

publishers_table = KikimrTable(db_name, *(publishers_data_model.serialize_to_common()[0]))


class Migration(BaseMigration):
    def execute(self) -> None:
        product_iterator = marketplace_db().with_table_scope(product=os_products_table, publishers=publishers_table). \
            select("SELECT os_product.id, publishers.name FROM $product_table as os_product "
                   "LEFT JOIN $publishers_table as publishers ON publishers.folder_id == os_product.folder_id "
                   "WHERE os_product.vendor IS NULL AND os_product.folder_id IS NOT NULL;")

        for product in product_iterator:
            marketplace_db().with_table_scope(os_products_table).update_object("UPDATE $table $set WHERE id=?", {
                "vendor": product.get("publishers.name"),
            }, product.get("os_product.id"), commit=True)

    def rollback(self):
        raise MigrationNoRollback
