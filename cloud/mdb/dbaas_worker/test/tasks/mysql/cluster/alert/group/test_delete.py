from test.mocks import get_state
from test.tasks.mysql.utils import get_mysql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised
from cloud.mdb.dbaas_worker.internal.providers.metadb_alert import MetadbAlert
from cloud.mdb.dbaas_worker.internal.providers.solomon_service_alerts import SolomonApiV2
from cloud.mdb.dbaas_worker.internal.providers.solomon_client.models import Template, Alert, AlertState


def _patch_solomon(mocker) -> SolomonApiV2:
    solomon = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.solomon_service_alerts.SolomonApiV2').return_value
    solomon._make_request.side_effect = lambda *_: None
    return solomon


def _patch_meta(mocker) -> MetadbAlert:
    meta_db = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.solomon_service_alerts.MetadbAlert').return_value
    to_create = 'to-create'
    to_delete = 'to-delete'
    to_modify = 'to-modify'
    meta_db.get_alerts_by_cid.side_effect = lambda *_: [
        Alert(
            ext_id='',
            name='',
            project_id='',
            state=AlertState.creating,
            template=Template(id=to_create, version=''),
            notification_channels=[],
            description='d1',
            alert_group_id='',
        ),
        Alert(
            ext_id='',
            name='',
            project_id='',
            state=AlertState.updating,
            template=Template(id=to_modify, version=''),
            notification_channels=[],
            description='d1',
            alert_group_id='',
        ),
        Alert(
            ext_id='',
            name='',
            project_id='',
            state=AlertState.deleting,
            template=Template(id=to_delete, version=''),
            notification_channels=[],
            description='d1',
            alert_group_id='',
        ),
    ]
    return meta_db


def test_mysql_alert_group_delete_interrupts(mocker):
    _patch_meta(mocker)
    _patch_solomon(mocker)
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
        },
        'target_host': 'host1',
    }

    check_task_interrupt_consistency(
        mocker,
        'mysql_alert_group_delete',
        args,
        get_state(),
    )


def test_mysql_alert_group_delete_mlock(mocker):
    _patch_meta(mocker)
    _patch_solomon(mocker)
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
        },
        'target_host': 'host1',
    }

    check_mlock_usage(
        mocker,
        'mysql_alert_group_delete',
        args,
        get_state(),
    )


def test_mysql_alert_group_delete_alert_sync(mocker):
    args = {}

    check_alerts_synchronised(
        mocker,
        'mysql_alert_group_delete',
        args,
        get_state(),
    )
