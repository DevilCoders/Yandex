"""
ClickHouse Database delete executor
"""
from ....common.cluster.database.delete import DatabaseDeleteExecutor
from ....utils import register_executor


@register_executor('clickhouse_database_delete')
class ClickHouseDatabaseDelete(DatabaseDeleteExecutor):
    """
    Delete clickhouse database
    """
