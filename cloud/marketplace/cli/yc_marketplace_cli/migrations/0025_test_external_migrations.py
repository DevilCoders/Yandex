from yc_marketplace_migrations.migration import BaseMigration


class Migration(BaseMigration):
    def execute(self):
        print("Nothing")

    def rollback(self):
        print("Nothing")
