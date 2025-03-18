import pytest
from antiadblock.postgres_local.migrator import FileBasedMigrator
from antiadblock.postgres_local.postgres import Config, PostgreSQL, State
from yatest.common.network import PortManager
from yatest.common import source_path


@pytest.yield_fixture(scope='session')
def postgresql_database():
    with PortManager() as pm:
        postgresql = None
        try:
            psql_config = Config(port=pm.get_port(5432),
                                 keep_work_dir=False,
                                 username='antiadb',
                                 password='postgres',
                                 dbname='configs')
            migrator = FileBasedMigrator(source_path('antiadblock/postgres_local/tests/test_migrations'))

            postgresql = PostgreSQL(psql_config, migrator)
            postgresql.run()
            postgresql.ensure_state(State.RUNNING)

            yield postgresql
        finally:
            if postgresql is not None:
                postgresql.shutdown()


def test_migrations(postgresql_database):
    def query(connection):
        names = [row[0] for row in connection.execute("SELECT name FROM users")]
        # Users added by 2 migrations from 'antiadblock/postgres_local/tests/test_migrations'
        assert 'test_user' in names
        assert 'test_user2' in names

    postgresql_database.exec_transaction(query)



