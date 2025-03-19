"""
SQLServer User create executor
"""

from ....common.cluster.user.create import UserCreateExecutor
from ....utils import register_executor


@register_executor('sqlserver_user_create')
class SQLServerUserCreate(UserCreateExecutor):
    """
    Create sqlserver user
    """
