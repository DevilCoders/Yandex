"""
Elasticsearch User modify executor
"""
from ....common.cluster.user.modify import UserModifyExecutor
from ....utils import register_executor


@register_executor('elasticsearch_user_modify')
class ElasticsearchUserModify(UserModifyExecutor):
    """
    Modify Elasticsearch user
    """
