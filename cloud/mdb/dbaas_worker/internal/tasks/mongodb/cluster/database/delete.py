"""
MongoDB Database delete executor
"""
from ....common.cluster.database.delete import DatabaseDeleteExecutor
from ....utils import register_executor


@register_executor('mongodb_database_delete')
class MongoDBDatabaseDelete(DatabaseDeleteExecutor):
    """
    Delete mongodb database
    """
