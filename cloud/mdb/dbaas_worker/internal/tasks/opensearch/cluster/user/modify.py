"""
Opensearch User modify executor
"""
from ....common.cluster.user.modify import UserModifyExecutor
from ....utils import register_executor


@register_executor('opensearch_user_modify')
class OpensearchUserModify(UserModifyExecutor):
    """
    Modify Opensearch user
    """
