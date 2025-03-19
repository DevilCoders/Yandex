# -*- coding: utf-8 -*-
"""
DBaaS Internal API MySQL alerts
"""

from typing import List

from ...core.types import Operation
from ...utils import metadb
from ...utils.alert_group import generate_alert_group_id, get_alerts_by_ag
from ...utils.host import get_hosts
from ...utils.metadata import AlertsGroupClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import Alert, AlertGroup, ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import MySQLOperations, MySQLTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.LIST)
def mysql_cluster_alert_groups(info: ClusterInfo) -> List[AlertGroup]:
    """
    Returns cluster alerts
    """
    alert_groups = metadb.list_alert_groups(cid=info.cid)

    return [
        AlertGroup(
            alerts=get_alerts_by_ag(ag["alert_group_id"]),
            **ag,
        )
        for ag in alert_groups
    ]


@register_request_handler(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.CREATE)
def mysql_create_alert_group(cluster: dict, monitoring_folder_id: str, alerts: List[dict], **_) -> Operation:
    """
    Create alert group
    """

    ag_id = generate_alert_group_id()

    metadb.add_alert_group(
        cid=cluster['cid'],
        ag_id=ag_id,
        monitoring_folder_id=monitoring_folder_id,
        managed=False,
    )

    for alert in alerts:
        metadb.add_alert_to_group(cid=cluster['cid'], alert=Alert(**alert), ag_id=ag_id)

    return create_operation(
        operation_type=MySQLOperations.alert_group_create,
        metadata=AlertsGroupClusterMetadata(alert_group_id=ag_id),
        cid=cluster['cid'],
        task_args={"hosts": get_hosts(cluster['cid'])},
        task_type=MySQLTasks.alert_group_create,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.DELETE)
def mysql_delete_alert_group(cluster: dict, alert_group_id: str, **_) -> Operation:
    """
    Delete alert group
    """

    metadb.delete_alert_group(alert_group_id=alert_group_id, cluster_id=cluster['cid'])

    return create_operation(
        operation_type=MySQLOperations.alert_group_delete,
        metadata=AlertsGroupClusterMetadata(alert_group_id=alert_group_id),
        cid=cluster['cid'],
        task_args={"hosts": get_hosts(cluster['cid'])},
        task_type=MySQLTasks.alert_group_delete,
    )
