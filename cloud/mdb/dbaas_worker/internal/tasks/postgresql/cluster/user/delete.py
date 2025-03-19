"""
PostgreSQL User delete executor
"""
from ....common.cluster.user.delete import UserDeleteExecutor
from ....utils import register_executor


@register_executor('postgresql_user_delete')
class PostgreSQLUserDelete(UserDeleteExecutor):
    """
    Delete postgresql user
    """
