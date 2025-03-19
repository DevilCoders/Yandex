"""
Apache Kafka User create executor
"""
from ....common.cluster.user.create import UserCreateExecutor
from ....utils import register_executor


@register_executor('kafka_user_create')
class KafkaUserCreate(UserCreateExecutor):
    """
    Create kafka user
    """
