"""
SqlServer Database delete executor
"""
from ....common.cluster.database.delete import DatabaseDeleteExecutor
from ....utils import register_executor


@register_executor('sqlserver_database_delete')
class SQLServerDatabaseDelete(DatabaseDeleteExecutor):
    """
    Delete sqlserver database
    """
