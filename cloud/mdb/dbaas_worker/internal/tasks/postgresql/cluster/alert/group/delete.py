"""
PostgreSQL alert management task
"""
from .....common.cluster.alert import AlertGroupExecutor
from .....utils import register_executor


@register_executor('postgresql_alert_group_delete')
class PostgreAlertGroupDelete(AlertGroupExecutor):
    pass
