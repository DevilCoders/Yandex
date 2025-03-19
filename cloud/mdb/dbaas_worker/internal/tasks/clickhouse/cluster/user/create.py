"""
ClickHouse User create executor
"""
from ....common.cluster.user.create import UserCreateExecutor
from ....utils import register_executor


@register_executor('clickhouse_user_create')
class ClickHouseUserCreate(UserCreateExecutor):
    """
    Create clickhouse user
    """
