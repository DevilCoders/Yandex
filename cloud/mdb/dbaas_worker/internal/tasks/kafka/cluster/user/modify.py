"""
Apache Kafka User modify executor
"""
from ....common.cluster.user.modify import UserModifyExecutor
from ....utils import register_executor


@register_executor('kafka_user_modify')
class KafkaUserModify(UserModifyExecutor):
    """
    Modify kafka user
    """
