"""
SQLServer User delete executor
"""

from ....common.cluster.user.delete import UserDeleteExecutor
from ....utils import register_executor


@register_executor('sqlserver_user_delete')
class SQLServerUserDelete(UserDeleteExecutor):
    """
    Delete sqlserver user
    """
