from yc_common.clients.kikimr import get_kikimr_client
from yc_marketplace_migrations.migration import BaseMigration


class Migration(BaseMigration):
    def execute(self):
        kikimr_client = get_kikimr_client("marketplace")
        queries = [
            """CREATE TABLE [sku_draft] (
                billing_account_id Utf8,
                id Utf8,
                status Utf8,
                publisher_account_id Utf8,
                name Utf8,
                description Utf8,
                pricing_versions Json,
                unit Utf8,
                created_at Uint64,
                updated_at Uint64,
                PRIMARY KEY (billing_account_id, id));
            """,
        ]
        for query in queries:
            kikimr_client.query(query, transactionless=True)

    def rollback(self):
        # Never drop table in rollback (backup it!)
        pass
