"""
ClickHouse User delete executor
"""
from ....common.cluster.user.delete import UserDeleteExecutor
from ....utils import register_executor


@register_executor('clickhouse_user_delete')
class ClickHouseUserDelete(UserDeleteExecutor):
    """
    Delete clickhouse user
    """
