"""
MongoDB User modify executor
"""
from ....common.cluster.user.modify import UserModifyExecutor
from ....utils import register_executor


@register_executor('mongodb_user_modify')
class MongoDBUserModify(UserModifyExecutor):
    """
    Modify mongodb user
    """
