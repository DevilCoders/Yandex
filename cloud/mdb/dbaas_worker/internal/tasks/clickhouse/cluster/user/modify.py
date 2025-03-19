"""
ClickHouse User modify executor
"""
from ....common.cluster.user.modify import UserModifyExecutor
from ....utils import register_executor


@register_executor('clickhouse_user_modify')
class ClickHouseUserModify(UserModifyExecutor):
    """
    Modify clickhouse user
    """
