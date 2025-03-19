from yc_common.clients.kikimr import get_kikimr_client
from yc_marketplace_migrations.migration import BaseMigration


class Migration(BaseMigration):
    def execute(self):
        kikimr_client = get_kikimr_client("marketplace")
        queries = [
            "ALTER TABLE [os_product] ADD COLUMN eula_id Utf8;",
            "ALTER TABLE [os_product] ADD COLUMN eula_uri Utf8;",
            "ALTER TABLE [saas_product] ADD COLUMN eula_uri Utf8;",
            "ALTER TABLE [saas_product] ADD COLUMN eula_id Utf8;",
            "ALTER TABLE [simple_product] ADD COLUMN eula_uri Utf8;",
            "ALTER TABLE [simple_product] ADD COLUMN eula_id Utf8;",
        ]
        for query in queries:
            kikimr_client.query(query, transactionless=True)

    def rollback(self):
        # Never drop table in rollback (backup it!)
        pass
