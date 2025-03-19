"""
MongoDB User delete executor
"""
from ....common.cluster.user.delete import UserDeleteExecutor
from ....utils import register_executor


@register_executor('mongodb_user_delete')
class MongoDBUserDelete(UserDeleteExecutor):
    """
    Delete mongodb user
    """
