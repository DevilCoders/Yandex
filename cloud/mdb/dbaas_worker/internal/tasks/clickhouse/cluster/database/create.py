"""
ClickHouse Database create executor
"""
from ....common.cluster.database.create import DatabaseCreateExecutor
from ....utils import register_executor


@register_executor('clickhouse_database_create')
class ClickHouseDatabaseCreate(DatabaseCreateExecutor):
    """
    Create clickhouse database
    """
