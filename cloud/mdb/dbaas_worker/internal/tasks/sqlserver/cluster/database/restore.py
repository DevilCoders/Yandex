"""
SqlServer Database restore executor
"""
from ....common.cluster.database.create import DatabaseCreateExecutor
from ....utils import register_executor


@register_executor('sqlserver_database_restore')
class SQLServerDatabaseRestore(DatabaseCreateExecutor):
    """
    Restore sqlserver database
    """

    def _make_pillar(self, host):
        return {
            'target-database': self.args['target-database'],
            'db-restore-from': self.args['db-restore-from'],
        }
