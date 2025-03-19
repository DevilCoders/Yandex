"""
Elasticsearch User delete executor
"""
from ....common.cluster.user.delete import UserDeleteExecutor
from ....utils import register_executor


@register_executor('elasticsearch_user_delete')
class ElasticsearchUserDelete(UserDeleteExecutor):
    """
    Delete Elasticsearch user
    """
