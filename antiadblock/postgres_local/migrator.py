import os

from sqlalchemy import text
from antiadblock.postgres_local.exceptions import MigrationException


class Migrator(object):
    def migrate(self, connection):
        """
        Concrete realization of migrations
        :param connection: psycopg2 connection to db
        :return:
        """
        pass


class FileBasedMigrator(Migrator):
    """
    Migrator that reads migration files from folder and applies them in one transaction. Assumes migrations follow in alphabetical order
    """
    def __init__(self, migrations_path, use_text=False):
        if not os.path.exists(migrations_path):
            raise MigrationException("path {} doesn't exist".format(migrations_path))
        if os.path.isfile(migrations_path):
            self.migration_files = [migrations_path]
        elif os.path.isdir(migrations_path):
            self.migration_files = sorted([os.path.join(migrations_path, file) for file in os.listdir(migrations_path)
                                           if os.path.isfile(os.path.join(migrations_path, file)) and file.endswith('.sql')])
        assert self.migration_files is not None
        self.use_text = use_text

    def migrate(self, connection):
        migrate_transaction = connection.begin()
        try:
            for file in self.migration_files:
                with open(file, 'r') as migration:
                    sql = migration.read()
                    if sql.strip() == '':
                        continue

                    if self.use_text:
                        sql = text(sql)

                    connection.execute(sql)

            migrate_transaction.commit()
        except:
            migrate_transaction.rollback()
            raise
