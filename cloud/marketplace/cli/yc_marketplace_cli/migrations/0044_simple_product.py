from yc_common.clients.kikimr import get_kikimr_client
from yc_marketplace_migrations.migration import BaseMigration


class Migration(BaseMigration):
    def execute(self):
        kikimr_client = get_kikimr_client("marketplace")
        queries = [
            "CREATE TABLE [product_slug] (product_id Utf8, created_at Uint64, slug Utf8, PRIMARY KEY (slug));",
            "INSERT INTO [product_slug] SELECT * FROM [os_product_slug];",
            "ALTER TABLE [product_slug] ADD COLUMN product_type Utf8;",
        ]
        for query in queries:
            kikimr_client.query(query, transactionless=True)
        kikimr_client = get_kikimr_client("marketplace")
        queries = [
            "UPDATE [product_slug] SET product_type='os'"
        ]
        for query in queries:
            kikimr_client.query(query, commit=True)
        kikimr_client = get_kikimr_client("marketplace")
        queries = [
            "CREATE TABLE [simple_product] (id Utf8, created_at Uint64, updated_at Uint64, billing_account_id Utf8, "
            "vendor Utf8, labels Json, name Utf8, description Utf8, short_description Utf8, logo_id Utf8, "
            "logo_uri Utf8, status Utf8, meta Json, score Double, PRIMARY KEY (id));"
        ]
        for query in queries:
            kikimr_client.query(query, transactionless=True)

    def rollback(self):
        # Never drop table in rollback (backup it!)
        pass
