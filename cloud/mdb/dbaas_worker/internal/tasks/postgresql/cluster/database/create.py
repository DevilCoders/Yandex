"""
PostgreSQL Database create executor
"""
from ....common.cluster.database.create import DatabaseCreateExecutor
from ....utils import register_executor


@register_executor('postgresql_database_create')
class PostgreSQLDatabaseCreate(DatabaseCreateExecutor):
    """
    Create postgresql database
    """
