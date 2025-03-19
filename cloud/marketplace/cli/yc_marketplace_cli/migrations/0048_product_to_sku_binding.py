from yc_common.clients.kikimr import get_kikimr_client
from yc_marketplace_migrations.migration import BaseMigration


class Migration(BaseMigration):
    def execute(self):
        kikimr_client = get_kikimr_client("marketplace")
        queries = [
            "CREATE TABLE [product_to_sku_binding] (product_id Utf8, sku_id Utf8, publisher_id Utf8, "
            "PRIMARY KEY (product_id, sku_id));",
            "ALTER TABLE [os_product_family_version] ADD COLUMN related_products Json;",
        ]
        for query in queries:
            kikimr_client.query(query, transactionless=True)

    def rollback(self):
        # Never drop table in rollback (backup it!)
        pass
