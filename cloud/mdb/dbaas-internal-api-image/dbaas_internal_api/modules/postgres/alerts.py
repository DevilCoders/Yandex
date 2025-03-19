# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL alerts
"""

from typing import List

from ...core.exceptions import AlertGroupDoesNotExists, DbaasClientError
from ...core.types import Operation
from ...utils import metadb
from ...utils.alert_group import generate_alert_group_id, get_alerts_by_ag
from ...utils.host import get_hosts
from ...utils.metadata import AlertsGroupClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import Alert, AlertGroup, ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import PostgresqlOperations, PostgresqlTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.LIST)
def postgresql_cluster_alert_groups(info: ClusterInfo) -> list[AlertGroup]:
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
def postgresql_create_alert_group(cluster: dict, monitoring_folder_id: str, alerts: List[dict], **_) -> Operation:
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
        operation_type=PostgresqlOperations.alert_group_create,
        metadata=AlertsGroupClusterMetadata(alert_group_id=ag_id),
        cid=cluster['cid'],
        task_args={"hosts": get_hosts(cluster['cid'])},
        # MDB-14755
        task_type=PostgresqlTasks.alert_group_create,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.MODIFY)
def postgresql_modify_alert_group(
    cluster: dict, alert_group_id: str, alerts: List[dict], monitoring_folder_id: str = None, **_
) -> Operation:
    """
    Modify alert group
    """

    cid = cluster['cid']
    ag = metadb.get_alert_group(cid=cid, ag_id=alert_group_id)
    if not ag:
        raise AlertGroupDoesNotExists(ag_id=alert_group_id)

    alerts_old = {
        alert['template_id']: Alert(**alert) for alert in metadb.get_alerts_by_alert_group(ag_id=alert_group_id)
    }

    if alerts:
        alerts_new = {alert['template_id']: Alert(**alert) for alert in alerts}

        add_alerts = set(alerts_new) - set(alerts_old)
        del_alerts = set(alerts_old) - set(alerts_new)
        upd_alerts = set(alerts_old).intersection(set(alerts_new))

        for alert_name in add_alerts:
            metadb.add_alert_to_group(cid=cid, alert=alerts_new[alert_name], ag_id=alert_group_id)

        for alert_name in del_alerts:
            metadb.delete_alert_from_group(
                cid=cid, template_id=alerts_old[alert_name].template_id, alert_group=alert_group_id
            )

        for alert_name in upd_alerts:
            if alerts_new[alert_name] != alerts_old[alert_name]:
                metadb.update_alert(
                    cid=cid,
                    alert_group_id=alert_group_id,
                    alert=alerts_new[alert_name],
                    alert_old=alerts_old[alert_name],
                )

        if monitoring_folder_id is not None and ag['monitoring_folder_id'] != monitoring_folder_id:
            raise DbaasClientError("modifying monitoring folder id is not supported yet")
            # TODO: MDB-13000 support it somehow
            # metadb.update_alert_group(cid=cid, ag_id=ag['alert_group_id'], monitoring_folder_id=monitoring_folder_id)

    return create_operation(
        operation_type=PostgresqlOperations.alert_group_modify,
        metadata=AlertsGroupClusterMetadata(alert_group_id=alert_group_id),
        cid=cluster['cid'],
        task_args={"hosts": get_hosts(cid)},
        # MDB-14755
        task_type=PostgresqlTasks.alert_group_modify,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.DELETE)
def postgresql_delete_alert_group(cluster: dict, alert_group_id: str, **_) -> Operation:
    """
    Delete alert group
    """

    metadb.delete_alert_group(alert_group_id=alert_group_id, cluster_id=cluster['cid'])

    return create_operation(
        operation_type=PostgresqlOperations.alert_group_delete,
        metadata=AlertsGroupClusterMetadata(alert_group_id=alert_group_id),
        cid=cluster['cid'],
        task_args={"hosts": get_hosts(cluster['cid'])},
        # MDB-14755
        task_type=PostgresqlTasks.alert_group_delete,
    )
