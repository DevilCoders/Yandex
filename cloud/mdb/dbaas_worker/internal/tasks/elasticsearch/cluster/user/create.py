"""
Elasticsearch User create executor
"""
from ....common.cluster.user.create import UserCreateExecutor
from ....utils import register_executor


@register_executor('elasticsearch_user_create')
class ElasticsearchUserCreate(UserCreateExecutor):
    """
    Create Elasticsearch user
    """
