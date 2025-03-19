from yc_common.clients.kikimr import get_kikimr_client
from yc_marketplace_migrations.migration import BaseMigration


class Migration(BaseMigration):
    def execute(self):
        kikimr_client = get_kikimr_client("marketplace")
        queries = [
            "CREATE TABLE [saas_product] (id Utf8, created_at Uint64, updated_at Uint64, billing_account_id Utf8, "
            "vendor Utf8, labels Json, name Utf8, description Utf8, short_description Utf8, logo_id Utf8, "
            "logo_uri Utf8, status Utf8, meta Json, score Double, sku_ids Json, PRIMARY KEY (id));"
        ]
        for query in queries:
            kikimr_client.query(query, transactionless=True)

    def rollback(self):
        # Never drop table in rollback (backup it!)
        pass
