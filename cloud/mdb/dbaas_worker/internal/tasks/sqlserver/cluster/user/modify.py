"""
SQLServer User modify executor
"""

from ....common.cluster.user.modify import UserModifyExecutor
from ....utils import register_executor


@register_executor('sqlserver_user_modify')
class SQLServerUserModify(UserModifyExecutor):
    """
    Modify sqlserver user
    """
