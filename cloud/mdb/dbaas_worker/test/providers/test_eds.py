from queue import Queue

from cloud.mdb.dbaas_worker.internal.providers.eds import EdsAlias, EdsApi

from mocks import _get_config, eds as mock_eds, get_state
from mocks.eds import DnsRecord


def test_alias_from_cname():
    full_cname = 'rc1a-221vhvr66c9n9fd6.db_e4ukih2munqqs7nskgf5.zookeeper.compute.preprod.mdb'
    alias = EdsAlias.from_full_cname(full_cname)
    expected = EdsAlias(
        host='rc1a-221vhvr66c9n9fd6',
        cid='db_e4ukih2munqqs7nskgf5',
        root='zookeeper',
        zone='compute.preprod.mdb',
    )
    assert alias == expected


def test_alias_to_cname():
    alias = EdsAlias(
        host='rc1a-221vhvr66c9n9fd6',
        cid='db_e4ukih2munqqs7nskgf5',
        root='zookeeper',
        zone='compute.preprod.mdb',
    )
    assert alias.discovery_mask == 'rc1a-221vhvr66c9n9fd6.db_e4ukih2munqqs7nskgf5.zookeeper.compute.preprod.mdb'
    assert alias.dns_name == 'rc1a-221vhvr66c9n9fd6.db_e4ukih2munqqs7nskgf5.zookeeper'


def get_eds_api():
    task = {
        'cid': 'cid-test',
        'task_id': 'test_id',
        'task_type': 'test-task',
        'feature_flags': [],
        'folder_id': 'test_folder',
        'context': {},
        'timeout': 3600,
        'changes': [],
    }
    config = _get_config()
    eds_api = EdsApi(config, task, queue=Queue(maxsize=10000))
    return eds_api


def test_register_host(mocker):
    state = get_state()
    mock_eds(mocker, state)
    assert state['eds'] == {'dns_cname_records': {}}

    eds_api = get_eds_api()

    fqdn = 'rc1a-0ve1lynrkubb5vb3.db.yandex.net'
    group = 'e4uv04vkci7iefuu8jj3'
    root = 'clickhouse'
    eds_api.host_register(fqdn, group, root)

    expected_record = DnsRecord(
        name='rc1a-0ve1lynrkubb5vb3.db_e4uv04vkci7iefuu8jj3.clickhouse',
        type='CNAME',
        ttl=300,
        data=['rc1a-0ve1lynrkubb5vb3.db.yandex.net'],
    )

    assert state['eds'] == {
        'dns_cname_records': {'rc1a-0ve1lynrkubb5vb3.db_e4uv04vkci7iefuu8jj3.clickhouse': expected_record}
    }


def test_unregister_host(mocker):
    state = get_state()
    old_record = DnsRecord(
        name='rc1a-0ve1lynrkubb5vb3.db_e4uv04vkci7iefuu8jj3.clickhouse',
        type='CNAME',
        ttl=300,
        data=['rc1a-0ve1lynrkubb5vb3.db.yandex.net'],
    )
    state['eds']['dns_cname_records'] = {'rc1a-0ve1lynrkubb5vb3.db_e4uv04vkci7iefuu8jj3.clickhouse': old_record}
    mock_eds(mocker, state)

    eds_api = get_eds_api()
    eds_api.host_unregister('rc1a-0ve1lynrkubb5vb3.db.yandex.net')

    assert state['eds'] == {'dns_cname_records': {}}


def test_modify_root(mocker):
    state = get_state()
    old_record = DnsRecord(
        name='rc1a-0ve1lynrkubb5vb3.db_e4uv04vkci7iefuu8jj3.clickhouse',
        type='CNAME',
        ttl=300,
        data=['rc1a-0ve1lynrkubb5vb3.db.yandex.net'],
    )
    state['eds']['dns_cname_records'] = {'rc1a-0ve1lynrkubb5vb3.db_e4uv04vkci7iefuu8jj3.clickhouse': old_record}
    mock_eds(mocker, state)

    eds_api = get_eds_api()
    new_root = 'test_alt_group'
    eds_api.group_has_root('e4uv04vkci7iefuu8jj3', new_root)

    expected_record = DnsRecord(
        name='rc1a-0ve1lynrkubb5vb3.db_e4uv04vkci7iefuu8jj3.test_alt_group',
        type='CNAME',
        ttl=300,
        data=['rc1a-0ve1lynrkubb5vb3.db.yandex.net'],
    )

    assert state['eds'] == {
        'dns_cname_records': {'rc1a-0ve1lynrkubb5vb3.db_e4uv04vkci7iefuu8jj3.test_alt_group': expected_record}
    }
