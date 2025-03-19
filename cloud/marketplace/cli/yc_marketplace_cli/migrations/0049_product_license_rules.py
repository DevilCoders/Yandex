from yc_common.clients.kikimr import get_kikimr_client
from yc_marketplace_migrations.migration import BaseMigration


class Migration(BaseMigration):
    def execute(self):
        kikimr_client = get_kikimr_client("marketplace")
        queries = [
            "CREATE TABLE [product_license_rules] (id Utf8, rules json, PRIMARY KEY (id));",
        ]
        for query in queries:
            kikimr_client.query(query, transactionless=True)

    def rollback(self):
        # Never drop table in rollback (backup it!)
        pass
