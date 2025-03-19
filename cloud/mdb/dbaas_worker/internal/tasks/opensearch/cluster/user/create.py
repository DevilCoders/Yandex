"""
Opensearch User create executor
"""
from ....common.cluster.user.create import UserCreateExecutor
from ....utils import register_executor


@register_executor('opensearch_user_create')
class OpensearchUserCreate(UserCreateExecutor):
    """
    Create Opensearch user
    """
