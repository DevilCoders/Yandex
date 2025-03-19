from yc_common.clients.kikimr import get_kikimr_client
from yc_marketplace_migrations.migration import BaseMigration


class Migration(BaseMigration):
    def execute(self):
        kikimr_client = get_kikimr_client("marketplace")
        queries = [
            """CREATE TABLE [builds] (
                id Utf8,
                blueprint_id Utf8,
                mkt_task_id Utf8,
                compute_image_id Utf8,
                blueprint_commit_hash Utf8,
                created_at Uint64,
                updated_at Uint64,
                status Utf8,
                PRIMARY KEY (blueprint_id, blueprint_commit_hash, id));
            """,
        ]
        for query in queries:
            kikimr_client.query(query, transactionless=True)

    def rollback(self):
        # Never drop table in rollback (backup it!)
        pass
