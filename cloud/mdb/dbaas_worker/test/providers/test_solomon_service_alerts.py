from queue import Queue

from pytest_mock import MockerFixture

from test.mocks import _get_config
from cloud.mdb.dbaas_worker.internal.providers.solomon_service_alerts import (
    sync_alerts,
    delete_by_cid,
    MetadbAlert,
    SolomonApiV2,
    Alert,
    AlertState,
)
from cloud.mdb.dbaas_worker.internal.providers.solomon_client.models import Template


def _get_task() -> dict:
    return {
        'cid': 'cid-test',
        'task_id': 'test_id',
        'task_type': 'test-task',
        'feature_flags': [],
        'folder_id': 'test_folder',
        'context': {},
        'timeout': 3600,
        'changes': [],
    }


def _get_metadb() -> MetadbAlert:
    config = _get_config()
    queue = Queue(maxsize=10000)
    return MetadbAlert(config, _get_task(), queue)


def _get_solomon_api(project_id: str) -> SolomonApiV2:
    config = _get_config()
    queue = Queue(maxsize=10000)
    return SolomonApiV2(config, _get_task(), queue, solomon_project=project_id)


def test_delete_alerts(mocker: MockerFixture):
    CID = 'test-cid'
    SOLOMON_PROJECT = 'test-solomon-prj'
    meta = _get_metadb()
    to_create = 'to-create'
    to_delete = 'to-delete'
    to_delete_2 = 'to-delete-2'
    to_modify = 'to-modify'

    def mock_get_alerts_by_cid(*_) -> list[Alert]:
        return [
            Alert(
                ext_id=to_create,
                name='',
                project_id='',
                state=AlertState.creating,
                template=Template(id=to_create, version=''),
                notification_channels=[],
                description='',
                alert_group_id='',
            ),
            Alert(
                ext_id=to_modify,
                name='',
                project_id='',
                state=AlertState.updating,
                template=Template(id=to_modify, version=''),
                notification_channels=[],
                description='',
                alert_group_id='',
            ),
            Alert(
                ext_id=to_delete,
                name='',
                project_id='',
                state=AlertState.deleting,
                template=Template(id=to_delete, version=''),
                notification_channels=[],
                description='',
                alert_group_id='',
            ),
            Alert(
                ext_id=to_delete_2,
                name='',
                project_id='',
                state=AlertState.active,
                template=Template(id=to_delete_2, version=''),
                notification_channels=[],
                description='',
                alert_group_id='',
            ),
        ]

    mocker.patch.object(meta, 'get_alerts_by_cid', mock_get_alerts_by_cid)

    def mock_get_cluster_project_id(*_) -> str:
        return ''

    mocker.patch.object(meta, 'get_cluster_project_id', mock_get_cluster_project_id)

    def noop(*_, **__):
        pass

    mocker.patch.object(meta, 'delete_alerts', noop)
    solomon = _get_solomon_api(SOLOMON_PROJECT)

    def solomon_request(*args, method=None, **kwargs) -> dict:
        if method == 'DELETE':
            return {}
        raise NotImplementedError

    mocker.patch.object(solomon, '_make_request', solomon_request)
    spy = mocker.spy(solomon, '_make_request')
    result = delete_by_cid(CID, meta, solomon)
    assert len(result.created) == 0
    assert len(result.modified) == 0
    assert len(result.deleted) == 4
    assert spy.call_count == 4
    assert len(result.untouched) == 0


def test_sync_alerts(mocker: MockerFixture):
    CID = 'test-cid'
    SOLOMON_PROJECT = 'test-solomon-prj'
    meta = _get_metadb()
    to_create = 'to-create'
    to_delete = 'to-delete'
    to_modify = 'to-modify'
    ACTUAL_TEMPLATE_VERSION_TAG = '2'
    OLD_TEMPLATE_VT = '1'

    def mock_get_alerts_by_cid(*_) -> list[Alert]:
        return [
            Alert(
                ext_id='1',
                name='',
                project_id='',
                state=AlertState.creating,
                template=Template(id=to_create, version=ACTUAL_TEMPLATE_VERSION_TAG),
                notification_channels=[],
                description='',
                alert_group_id='',
            ),
            Alert(
                ext_id='2',
                name='',
                project_id='',
                state=AlertState.updating,
                template=Template(id=to_modify, version=ACTUAL_TEMPLATE_VERSION_TAG),
                notification_channels=[],
                description='',
                alert_group_id='',
            ),
            Alert(
                ext_id='3',
                name='',
                project_id='',
                state=AlertState.deleting,
                template=Template(id=to_delete, version=ACTUAL_TEMPLATE_VERSION_TAG),
                notification_channels=[],
                description='',
                alert_group_id='',
            ),
            Alert(
                ext_id='4',
                name='',
                project_id='',
                state=AlertState.active,
                template=Template(id=to_delete, version=ACTUAL_TEMPLATE_VERSION_TAG),
                notification_channels=[],
                description='',
                alert_group_id='',
            ),
        ]

    mocker.patch.object(meta, 'get_alerts_by_cid', mock_get_alerts_by_cid)

    def mock_get_cluster_project_id(*_) -> str:
        return ''

    mocker.patch.object(meta, 'get_cluster_project_id', mock_get_cluster_project_id)

    def noop(*_, **__):
        pass

    mocker.patch.object(meta, 'set_alerts_active', noop)
    mocker.patch.object(meta, 'delete_alerts', noop)
    solomon = _get_solomon_api(SOLOMON_PROJECT)

    def solomon_request(*args, method=None, **kwargs) -> dict:
        if method == 'POST':
            if args[0] == '/alerts/2/update-template-version':
                return {}
            elif args[0] == '/alerts':
                return {'id': 'test-id'}
        elif method == 'DELETE':
            return {}
        elif method == 'PUT':
            if kwargs['data']['type']['fromTemplate']['templateVersionTag'] != ACTUAL_TEMPLATE_VERSION_TAG:
                raise ValueError('cannot update with not actual version')
            return {}
        elif method == 'GET':
            if args[0] == '/alerts/2':
                return dict(type=dict(fromTemplate=dict(templateVersionTag=OLD_TEMPLATE_VT)))
        raise NotImplementedError

    mocker.patch.object(solomon, '_make_request', solomon_request)
    spy = mocker.spy(solomon, '_make_request')
    result = sync_alerts(CID, meta, solomon)
    assert len(result.created) == 1
    alert = result.created.pop()
    assert alert.template.id == to_create
    assert alert.ext_id == 'test-id'
    assert len(result.modified) == 1
    alert = result.modified.pop()
    assert alert.template.id == to_modify
    assert len(result.deleted) == 1
    alert = result.deleted.pop()
    assert alert.template.id == to_delete
    assert spy.call_count == 5
    assert len(result.untouched) == 1
