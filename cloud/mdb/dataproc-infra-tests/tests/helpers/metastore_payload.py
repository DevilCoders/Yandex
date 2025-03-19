from hive_metastore_client import HiveMetastoreClient
from hive_metastore_client.builders import DatabaseBuilder
from hive_metastore_client.builders import (
    ColumnBuilder,
    SerDeInfoBuilder,
    StorageDescriptorBuilder,
    TableBuilder,
)
from thrift_files.libraries.thrift_hive_metastore_client.ttypes import FieldSchema, NoSuchObjectException


def test_metastore(metastore_hostname, metastore_port=9083):
    with HiveMetastoreClient(metastore_hostname, metastore_port) as metastore_client:
        database_name = 'my_test_db'
        try:
            metastore_client.get_database(database_name)
            metastore_client.drop_database(database_name, deleteData=True, cascade=True)
        except NoSuchObjectException:
            pass

        database_create_request = DatabaseBuilder(name=database_name).build()
        metastore_client.create_database(database_create_request)
        database = metastore_client.get_database(database_name)
        assert database.name == database_name

        tables = metastore_client.get_all_tables(database_name)
        assert tables == []

        owner_name = 'john_from_602'
        table_name = 'my_test_table'
        table_create_request = TableBuilder(
            table_name=table_name,
            db_name=database_name,
            owner=owner_name,
            storage_descriptor=StorageDescriptorBuilder(
                columns=[
                    ColumnBuilder("id", "string", "col comment").build(),
                    ColumnBuilder("client_name", "string").build(),
                    ColumnBuilder("amount", "string").build(),
                    ColumnBuilder("year", "string").build(),
                    ColumnBuilder("month", "string").build(),
                    ColumnBuilder("day", "string").build(),
                ],
                location="/path/to/file",
                input_format="org.apache.hadoop.hive.ql.io.parquet.MapredParquetInputFormat",
                output_format="org.apache.hadoop.hive.ql.io.parquet.MapredParquetOutputFormat",
                serde_info=SerDeInfoBuilder(
                    serialization_lib="org.apache.hadoop.hive.ql.io.parquet.serde.ParquetHiveSerDe"
                ).build(),
            ).build(),
            partition_keys=[
                ColumnBuilder("year", "string").build(),
                ColumnBuilder("month", "string").build(),
                ColumnBuilder("day", "string").build(),
            ],
        ).build()
        metastore_client.create_table(table_create_request)

        tables = metastore_client.get_all_tables(database_name)
        assert tables == [table_name]

        metastore_table = metastore_client.get_table(database_name, table_name)
        assert metastore_table.owner == owner_name

        partition_keys_names = metastore_client.get_partition_keys_names(database_name, table_name)
        assert partition_keys_names == ['year', 'month', 'day']

        partition_keys_objects = metastore_client.get_partition_keys_objects(database_name, table_name)
        assert partition_keys_objects == [
            FieldSchema(name='year', type='string', comment=None),
            FieldSchema(name='month', type='string', comment=None),
            FieldSchema(name='day', type='string', comment=None),
        ]

        partition_keys = metastore_client.get_partition_keys(database_name, table_name)
        assert partition_keys == [('year', 'string'), ('month', 'string'), ('day', 'string')]

        partition_values = metastore_client.get_partition_values_from_table(database_name, table_name)
        assert partition_values == []

        metastore_client.drop_table(database_name, table_name, deleteData=True)
        tables = metastore_client.get_all_tables(database_name)
        assert tables == []

        metastore_client.drop_database(database_name, deleteData=True, cascade=True)

        return True


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--hostname', type=str, help='metastore hostname')
    parser.add_argument('--port', type=int, help='metastore port', default=9083)
    args = parser.parse_args()

    print('all tests have passed') if test_metastore(
        metastore_hostname=args.hostname,
        metastore_port=args.port,
    ) is True else None
