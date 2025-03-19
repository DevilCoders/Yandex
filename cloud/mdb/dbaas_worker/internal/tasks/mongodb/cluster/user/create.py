"""
MongoDB User create executor
"""
from ....common.cluster.user.create import UserCreateExecutor
from ....utils import register_executor


@register_executor('mongodb_user_create')
class MongoDBUserCreate(UserCreateExecutor):
    """
    Create mongodb user
    """
