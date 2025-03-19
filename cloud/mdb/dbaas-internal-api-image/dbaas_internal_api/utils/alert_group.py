"""
Alert id
"""
from .types import Alert
from ..core.exceptions import DbaasClientError
from ..core.id_generators import gen_id
from ..utils import metadb


def generate_alert_group_id() -> str:
    """
    Generate backup_id
    """
    return gen_id('alert_group')


def get_alerts_by_ag(alert_group_id):
    """
    List alerts
    """
    alerts = metadb.get_alerts_by_alert_group(ag_id=alert_group_id)

    return list(sorted(alerts, key=lambda x: x['template_id']))


def create_default_alert_group(cid: str, alert_group_spec: dict):
    """
    Create default alert group
    """

    if 'monitoring_folder_id' not in alert_group_spec:
        raise DbaasClientError("monitoring folder id should be specified")

    ag_id = generate_alert_group_id()
    metadb.add_alert_group(
        cid=cid,
        managed=True,
        ag_id=ag_id,
        monitoring_folder_id=alert_group_spec["monitoring_folder_id"],
    )

    for alert in alert_group_spec["alerts"]:
        metadb.add_alert_to_group(ag_id=ag_id, alert=Alert(**alert), cid=cid)

    return ag_id
