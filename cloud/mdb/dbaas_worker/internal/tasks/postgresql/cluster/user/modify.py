"""
PostgreSQL User modify executor
"""
from ....common.cluster.user.modify import UserModifyExecutor
from ....utils import register_executor


@register_executor('postgresql_user_modify')
class PostgreSQLUserModify(UserModifyExecutor):
    """
    Modify postgresql user
    """
