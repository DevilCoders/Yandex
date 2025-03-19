"""
Solomon Service Alerts interaction module
"""


from dataclasses import dataclass
import itertools

from .http import BaseProvider
from ..exceptions import ExposedException
from .metadb_alert import MetadbAlert
from .solomon_client import SolomonApiV2, SolomonApiV3
from .solomon_client.models import Alert, AlertState

STATES_TO_CREATE = {AlertState.creating, AlertState.create_error}
STATES_TO_DELETE = {AlertState.deleting, AlertState.delete_error}
STATES_TO_MODIFY = {AlertState.updating}


class SolomonServiceAlertApiError(ExposedException):
    """
    Base solomon service alerts error
    """


class SolomonServiceAlerts(BaseProvider):
    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.meta_db = MetadbAlert(config, task, queue)

    def delete_by_cid(self, cid: str):
        solomon_api = SolomonApiV2(self.config, self.task, self.queue)
        delete_by_cid(cid, self.meta_db, solomon_api)

    def state_to_desired(self, cid: str):
        solomon_api = SolomonApiV2(self.config, self.task, self.queue)
        sync_alerts(cid, self.meta_db, solomon_api)


class SolomonServiceAlertsV2(BaseProvider):
    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)

    def state_to_desired(self, cid: str, monitoring_stub_request_id: str):
        solomon_api = SolomonApiV3(self.config, self.task, self.queue)
        solomon_api.complete_stud_request(cid, monitoring_stub_request_id)


def filter_alerts_to_create(alerts: list[Alert]) -> set[Alert]:
    result = set()
    for alert in alerts:
        if alert.state in STATES_TO_CREATE:
            result.add(alert)
    return result


def filter_alerts_to_delete(alerts: list[Alert]) -> list[Alert]:
    result = []
    for alert in alerts:
        if alert.state in STATES_TO_DELETE:
            result.append(alert)
    return result


def filter_alerts_to_modify(alerts: list[Alert]) -> set[Alert]:
    result = set()
    for alert in alerts:
        if alert.state in STATES_TO_MODIFY:
            result.add(alert)
    return result


@dataclass
class SyncResult:
    created: set[Alert]
    modified: set[Alert]
    deleted: list[Alert]
    untouched: set[Alert]


def _delete_by_labels(cid: str, prj_id: str, solomon: SolomonApiV2):
    aids = solomon.alerts_list_by_labels(cid, prj_id)
    if len(aids) > 0:
        solomon.delete_alerts_by_id(aids)


def delete_by_cid(cid: str, metadb: MetadbAlert, solomon: SolomonApiV2) -> SyncResult:
    all_alerts = metadb.get_alerts_by_cid(cid)
    if len(all_alerts) > 0:
        solomon.delete_alerts_by_id(all_alerts)

        def group_by_alert_group_id(item: Alert) -> str:
            return item.alert_group_id

        all_alerts.sort(key=group_by_alert_group_id)

        grouping = itertools.groupby(all_alerts, group_by_alert_group_id)
        for alert_group_id, alerts in grouping:
            metadb.delete_alerts(alert_group_id, [a.template.id for a in alerts])

    prj_id = metadb.get_cluster_project_id(cid)
    if prj_id:
        _delete_by_labels(cid, prj_id, solomon)

    return SyncResult(
        created=set(),
        modified=set(),
        deleted=all_alerts,
        untouched=set(),
    )


def sync_alerts(cid: str, metadb: MetadbAlert, solomon: SolomonApiV2) -> SyncResult:
    all_alerts = metadb.get_alerts_by_cid(cid)

    to_create = filter_alerts_to_create(all_alerts)
    solomon.create_service_alerts(to_create, cid)

    to_modify = filter_alerts_to_modify(all_alerts)
    solomon.modify_alerts(to_modify, cid)
    set_active = to_create.union(to_modify)
    if len(set_active) > 0:
        metadb.set_alerts_active(set_active)

    to_delete = filter_alerts_to_delete(all_alerts)
    solomon.delete_alerts_by_id(to_delete)
    if len(to_delete) > 0:

        def group_by_alert_group_id(item: Alert) -> str:
            return item.alert_group_id

        to_delete.sort(key=group_by_alert_group_id)

        grouping = itertools.groupby(to_delete, group_by_alert_group_id)
        for alert_group_id, alerts in grouping:
            metadb.delete_alerts(alert_group_id, [a.template.id for a in alerts])

    untouched = set(all_alerts) - set(to_delete) - to_create - to_modify
    return SyncResult(
        created=to_create,
        modified=to_modify,
        deleted=to_delete,
        untouched=untouched,
    )
