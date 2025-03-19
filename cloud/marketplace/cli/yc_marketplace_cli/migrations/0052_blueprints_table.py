from yc_common.clients.kikimr import get_kikimr_client
from yc_marketplace_migrations.migration import BaseMigration


class Migration(BaseMigration):
    def execute(self):
        kikimr_client = get_kikimr_client("marketplace")
        queries = [
            """CREATE TABLE [blueprints] (
                id Utf8,
                status Utf8,
                publisher_account_id Utf8,
                name Utf8,
                created_at Uint64,
                updated_at Uint64,
                build_recipe_links Json,
                test_suites_links Json,
                test_instance_config Json,
                PRIMARY KEY (publisher_account_id, id));
            """,
        ]
        for query in queries:
            kikimr_client.query(query, transactionless=True)

    def rollback(self):
        # Never drop table in rollback (backup it!)
        pass
