"""
Apache Kafka User delete executor
"""
from ....common.cluster.user.delete import UserDeleteExecutor
from ....utils import register_executor


@register_executor('kafka_user_delete')
class KafkaUserDelete(UserDeleteExecutor):
    """
    Delete kafka user
    """
