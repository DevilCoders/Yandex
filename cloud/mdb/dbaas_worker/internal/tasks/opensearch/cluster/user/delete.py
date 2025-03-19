"""
Opensearch User delete executor
"""
from ....common.cluster.user.delete import UserDeleteExecutor
from ....utils import register_executor


@register_executor('opensearch_user_delete')
class OpensearchUserDelete(UserDeleteExecutor):
    """
    Delete Opensearch user
    """
