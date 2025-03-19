"""
MySQL alert management task
"""
from .....common.cluster.alert import AlertGroupExecutor
from .....utils import register_executor


@register_executor('mysql_alert_group_create')
class MysqlAlertGroupCreate(AlertGroupExecutor):
    pass
