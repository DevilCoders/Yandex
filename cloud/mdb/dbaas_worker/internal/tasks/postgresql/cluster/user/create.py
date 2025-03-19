"""
PostgreSQL User create executor
"""
from ....common.cluster.user.create import UserCreateExecutor
from ....utils import register_executor


@register_executor('postgresql_user_create')
class PostgreSQLUserCreate(UserCreateExecutor):
    """
    Create postgresql user
    """
