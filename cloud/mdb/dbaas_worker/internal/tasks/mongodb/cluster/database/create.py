"""
MongoDB Database create executor
"""
from ....common.cluster.database.create import DatabaseCreateExecutor
from ....utils import register_executor


@register_executor('mongodb_database_create')
class MongoDBDatabaseCreate(DatabaseCreateExecutor):
    """
    Create roles and sync users.
    FYI: it does not create db in mongodb, because mongodb does not support empty db
    """
