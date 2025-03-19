"""
SQLServer Database modify executor
"""
from ....common.cluster.database.modify import DatabaseModifyExecutor
from ....utils import register_executor


@register_executor('sqlserver_database_modify')
class SQLServerDatabaseModify(DatabaseModifyExecutor):
    """
    Modify sqlserver database
    """
