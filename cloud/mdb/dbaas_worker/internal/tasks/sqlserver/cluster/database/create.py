"""
SqlServer Database create executor
"""
from ....common.cluster.database.create import DatabaseCreateExecutor
from ....utils import register_executor


@register_executor('sqlserver_database_create')
class SQLServerDatabaseCreate(DatabaseCreateExecutor):
    """
    Create sqlserver database
    """
