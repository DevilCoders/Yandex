"""
PostgreSQL Database delete executor
"""
from ....common.cluster.database.delete import DatabaseDeleteExecutor
from ....utils import register_executor


@register_executor('postgresql_database_delete')
class PostgreSQLDatabaseDelete(DatabaseDeleteExecutor):
    """
    Delete postgresql database
    """
