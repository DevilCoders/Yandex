import logging

from cloud.mdb.dbaas_worker.internal.providers.metadb_alert import MetadbAlert
from cloud.mdb.dbaas_worker.internal.providers.solomon_client.models import AlertState, Alert, Template
from cloud.mdb.dbaas_worker.internal.metadb import get_cursor
from .db import (  # noqa: F401
    empty_metadb,
    ConnectionCfg,
    CLUSTER_ID,
    FOLDER_ID,
    TEMPLATE_ID,
    TEMPLATE_VERSION,
    ALERT_GROUP_ID,
    metadb_with_alert_group,
    metadb_alerts_provider,
)

log = logging.getLogger(__name__)


def test_alerts_read(metadb_with_alert_group, metadb_alerts_provider: MetadbAlert):  # noqa: F811
    with get_cursor(metadb_with_alert_group.conn_string, metadb_with_alert_group.hosts, log) as cursor:
        create_alert(cursor, CLUSTER_ID, ALERT_GROUP_ID, TEMPLATE_ID)
    alerts = metadb_alerts_provider.get_alerts_by_cid(CLUSTER_ID)
    assert len(alerts) == 1
    alert = alerts[0]
    assert alert.template.id == TEMPLATE_ID
    assert alert.state == AlertState.creating
    assert alert.ext_id == ''
    assert alert.notification_channels == ['test-ch-1']
    assert alert.project_id == 'solomon-project-id'


ALERT_EXT_ID = 'test-ext-id'
ALERT_NAME = 'test_name_cid'
ALERT_DESCRIPTION = 'test_alert_description'


def create_alert(cursor, cid: str, alert_group_id: str, template_id: str):
    sql = '''SELECT * FROM code.lock_cluster(%(cid)s, %(req)s)'''
    cursor.execute(
        sql,
        dict(
            req='test-req-id',
            cid=cid,
        ),
    )
    cluster = cursor.fetchall()[0]
    sql = '''
    SELECT * FROM code.add_alert_to_group(
        i_cid := %(cid)s,
        i_alert_group_id := %(alert_group_id)s,
        i_template_id := %(template_id)s,
        i_crit_threshold := %(crit_threshold)s,
        i_warn_threshold := %(warn_threshold)s,
        i_disabled := %(disabled)s,
        i_notification_channels := %(notification_channels)s,
        i_default_thresholds := %(default_thresholds)s,
        i_rev := %(rev)s
    )'''
    cursor.execute(
        sql,
        dict(
            cid=cluster['cid'],
            alert_group_id=alert_group_id,
            template_id=template_id,
            rev=cluster['rev'],
            disabled=False,
            default_thresholds=True,
            warn_threshold=50,
            crit_threshold=99,
            notification_channels=["test-ch-1"],
        ),
    )
    sql = '''
            SELECT *
            FROM code.complete_cluster_change (
                i_cid := %(cid)s,
                i_rev := %(rev)s
            )'''
    cursor.execute(
        sql,
        dict(
            cid=CLUSTER_ID,
            rev=cluster['rev'],
        ),
    )
    cursor.execute('COMMIT')


def test_alerts_activated(metadb_with_alert_group: ConnectionCfg, metadb_alerts_provider: MetadbAlert):  # noqa: F811
    with get_cursor(metadb_with_alert_group.conn_string, metadb_with_alert_group.hosts, log) as cursor:
        create_alert(cursor, CLUSTER_ID, ALERT_GROUP_ID, TEMPLATE_ID)
        alert_to_set = Alert(
            ext_id=ALERT_EXT_ID,
            name=ALERT_NAME,
            project_id=FOLDER_ID,
            state=AlertState.creating,
            template=Template(id=TEMPLATE_ID, version=TEMPLATE_VERSION),
            notification_channels=[],
            description=ALERT_DESCRIPTION,
            alert_group_id=ALERT_GROUP_ID,
        )
        metadb_alerts_provider.set_alerts_active([alert_to_set])
        alerts = metadb_alerts_provider.get_alerts_by_cid(CLUSTER_ID)
        assert len(alerts) == 1
        alert = alerts[0]
        assert alert.template.id == TEMPLATE_ID
        assert alert.state == AlertState.active
        assert alert.ext_id == ALERT_EXT_ID
        assert alert.notification_channels == ['test-ch-1']
        assert alert.project_id == 'solomon-project-id'
        assert alert.alert_group_id == ALERT_GROUP_ID


def _alert_deleting(cursor, group_id: str, template_id: str):
    sql = '''
    UPDATE dbaas.alert SET status = 'DELETING' WHERE alert_group_id = %(group_id)s AND template_id = %(template_id)s
    '''
    cursor.execute(
        sql,
        dict(
            group_id=group_id,
            template_id=template_id,
        ),
    )


def test_alerts_deleted(metadb_with_alert_group: ConnectionCfg, metadb_alerts_provider: MetadbAlert):  # noqa: F811
    with get_cursor(metadb_with_alert_group.conn_string, metadb_with_alert_group.hosts, log) as cursor:
        create_alert(cursor, CLUSTER_ID, ALERT_GROUP_ID, TEMPLATE_ID)
        _alert_deleting(cursor, ALERT_GROUP_ID, TEMPLATE_ID)
        alerts = metadb_alerts_provider.get_alerts_by_cid(CLUSTER_ID)
        assert len(alerts) == 1
        assert alerts[0].state == AlertState.deleting
        metadb_alerts_provider.delete_alerts(ALERT_GROUP_ID, [TEMPLATE_ID])
        alerts = metadb_alerts_provider.get_alerts_by_cid(CLUSTER_ID)
        assert len(alerts) == 0
