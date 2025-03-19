import os

import psycopg2


def dsn():
    host = os.environ.get('METADB_POSTGRESQL_RECIPE_HOST')
    port = os.environ.get('METADB_POSTGRESQL_RECIPE_PORT')
    return f'host={host} port={port} dbname=dbaas_metadb user=dbaas_api'


def row_count_in_table(table_name):
    with psycopg2.connect(dsn()) as conn:
        cursor = conn.cursor()
        cursor.execute(f'SELECT count(*) FROM {table_name}')  # noqa
        return cursor.fetchone()[0]


class Test_data:
    def test_flavor_type(self):
        assert row_count_in_table('dbaas.flavor_type') == 11

    def test_flavors(self):
        assert row_count_in_table('dbaas.flavors') == 107

    def test_geo(self):
        assert row_count_in_table('dbaas.geo') == 12

    def test_disk_type(self):
        assert row_count_in_table('dbaas.disk_type') == 5

    def test_default_pillar(self):
        assert row_count_in_table('dbaas.default_pillar') == 1

    def test_valid_resources(self):
        assert (
            row_count_in_table('dbaas.valid_resources') > 3000
        ), 'there are a lot of **generated** rows in valid_resources'
