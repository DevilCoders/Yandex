"""
Test for MDB Health client
"""

import pytest
from hamcrest import assert_that, has_properties

from dbaas_internal_api.health.health import (
    HealthInfo,
    HostHealth,
    InvalidHealthData,
    MDBHealth,
    MDBHealthProviderHTTP,
    ServiceReplicaType,
    ServiceRole,
    ServiceStatus,
    service_replica_type_from_mdbh,
    service_role_from_mdbh,
    service_status_from_mdbh,
)
from ..mocks import mock_mdb_health_from_memory


def test_service_role_from_mdbh():
    assert service_role_from_mdbh('Unknown') == ServiceRole.unknown
    assert service_role_from_mdbh('Master') == ServiceRole.master
    assert service_role_from_mdbh('Replica') == ServiceRole.replica
    assert service_role_from_mdbh('SomeRandomStuff') == ServiceRole.unknown
    assert service_role_from_mdbh('') == ServiceRole.unknown


def test_service_replica_type_from_mdbh():
    assert service_replica_type_from_mdbh('Unknown') == ServiceReplicaType.unknown
    assert service_replica_type_from_mdbh('Async') == ServiceReplicaType.a_sync
    assert service_replica_type_from_mdbh('Sync') == ServiceReplicaType.sync
    assert service_replica_type_from_mdbh('StrangeReplicaType') == ServiceReplicaType.unknown
    assert service_replica_type_from_mdbh('') == ServiceReplicaType.unknown


def test_service_status_from_mdbh():
    assert service_status_from_mdbh('Unknown') == ServiceStatus.unknown
    assert service_status_from_mdbh('Alive') == ServiceStatus.alive
    assert service_status_from_mdbh('Dead') == ServiceStatus.dead
    assert service_status_from_mdbh('SomeRandomStuff') == ServiceStatus.unknown
    assert service_status_from_mdbh('') == ServiceStatus.unknown


valid_services = [
    {
        'name': 'MasterAlive',
        'status': 'Alive',
        'role': 'Master',
    },
    {
        'name': 'MasterDead',
        'status': 'Dead',
        'role': 'Master',
    },
    {
        'name': 'UnknownMaster',
        'status': 'Unknown',
        'role': 'Master',
    },
    {
        'name': 'ReplicaAlive',
        'status': 'Alive',
        'role': 'Replica',
    },
    {
        'name': 'ReplicaDead',
        'status': 'Dead',
        'role': 'Replica',
    },
    {
        'name': 'ReplicaMaster',
        'status': 'Unknown',
        'role': 'Replica',
    },
    {
        'name': 'UnknownAlive',
        'status': 'Alive',
        'role': 'Unknown',
    },
    {
        'name': 'UnknownDead',
        'status': 'Dead',
        'role': 'Unknown',
    },
    {
        'name': 'UnknownMaster',
        'status': 'Unknown',
        'role': 'Unknown',
    },
]

invalid_services = [
    {
        'name': 'nameNotAlive',
        'status': 'NotAlive',
        'role': 'Something',
    },
    {
        'name': 'nameNotDead',
        'status': 'NotDead',
        'role': 'Something',
    },
    {
        'name': 'nameNotUnknown',
        'status': 'NotUnknown',
        'role': 'Something',
    },
    {
        'name': 'nameAlive',
        'status': 'Alive',
    },
    {
        'role': 'Master',
        'status': 'Alive',
    },
    {
        'name': 'nameMaster',
        'role': 'Master',
    },
]

valid_hosts = [
    {
        'cid': 'cid',
        'fqdn': 'fqdn',
    },
]

valid_system_metrics = {
    'cpu': {
        'timestamp': 1000,
        'used': 0.1,
    },
    'mem': {
        'timestamp': 1000,
        'used': 100,
        'total': 1000,
    },
    'disk': {
        'timestamp': 1000,
        'used': 2000,
        'total': 30000,
    },
}

valid_system_metrics_without_field = {
    'cpu': {
        'timestamp': 1000,
        'used': 0.1,
    },
    'disk': {
        'timestamp': 1000,
        'used': 2000,
        'total': 30000,
    },
}

empty_system_metrics = {}

invalid_system_metrics = {
    'cpu': {
        'timestamp': 1000,
    },
    'mem': {
        'used': 100,
        'total': 1000,
    },
    'disk': {
        'timestamp': 1000,
        'used': 2000,
        'total': 30000,
    },
}

invalid_hosts = [
    {
        'fqdn': 'fqdn',
    },
    {
        'cid': 'cid',
    },
]


def populate_host_healths(hosts, services, system):
    resp = []
    for host in hosts:
        copy = host.copy()
        copy['services'] = services
        copy['system'] = system
        resp.append(copy)
    return resp


@pytest.mark.parametrize(
    'hosts',
    populate_host_healths(valid_hosts, valid_services, valid_system_metrics)
    + populate_host_healths(valid_hosts, valid_services, valid_system_metrics_without_field)
    + populate_host_healths(valid_hosts, valid_services, empty_system_metrics),
)
def test_valid_host_health(mocker, hosts):
    HostHealth(hosts)


@pytest.mark.parametrize(
    'hosts',
    populate_host_healths(valid_hosts, invalid_services, valid_system_metrics)
    + populate_host_healths(invalid_hosts, valid_services, valid_system_metrics)
    + populate_host_healths(invalid_hosts, valid_services, invalid_system_metrics)
    + populate_host_healths(valid_hosts, invalid_services, invalid_system_metrics)
    + populate_host_healths(valid_hosts, valid_services, invalid_system_metrics),
)
def test_invalid_host_health(hosts):
    with pytest.raises(InvalidHealthData):
        HostHealth(hosts)


@pytest.mark.parametrize(
    'hosts',
    [
        populate_host_healths(valid_hosts, valid_services, valid_system_metrics)
        + populate_host_healths(valid_hosts, valid_services, valid_system_metrics_without_field)
        + populate_host_healths(valid_hosts, valid_services, empty_system_metrics)
    ],
)
def test_valid_health_info(hosts):
    HealthInfo(hosts)


@pytest.mark.parametrize(
    'hosts',
    [
        populate_host_healths(valid_hosts, invalid_services, valid_system_metrics)
        + populate_host_healths(invalid_hosts, valid_services, valid_system_metrics)
        + populate_host_healths(invalid_hosts, valid_services, invalid_system_metrics)
        + populate_host_healths(valid_hosts, invalid_services, invalid_system_metrics)
        + populate_host_healths(valid_hosts, valid_services, invalid_system_metrics)
    ],
)
def test_invalid_health_info(hosts):
    with pytest.raises(InvalidHealthData):
        HealthInfo(hosts)


def test_mdb_health_client(mocker):
    fqdn = 'fqdn'
    cid = 'cid'
    logger = 'fake'
    system_ts = 1000
    system_cpu_used = 0.1
    system_memory_used = 100
    system_memory_total = 1000
    system_disk_used = 2000
    system_disk_total = 30000
    expected = HealthInfo(
        [
            {
                'fqdn': fqdn,
                'cid': cid,
                'services': [
                    {
                        'name': 'name',
                        'status': 'Alive',
                        'role': 'Master',
                    }
                ],
                'system': {
                    'cpu': {
                        'timestamp': system_ts,
                        'used': system_cpu_used,
                    },
                    'mem': {
                        'timestamp': system_ts,
                        'used': system_memory_used,
                        'total': system_memory_total,
                    },
                    'disk': {
                        'timestamp': system_ts,
                        'used': system_disk_used,
                        'total': system_disk_total,
                    },
                },
            },
        ],
    )

    mock_mdb_health_from_memory(mocker, expected)

    mdbh = MDBHealth()
    mdbh.init_mdbhealth(
        MDBHealthProviderHTTP({'url': 'unknown', 'connect_timeout': 0, 'read_timeout': 0, 'ca_certs': 'nocert'}),
        logger,
    )

    actual = mdbh.health_by_fqdns([fqdn])
    assert actual == expected
    host = actual.host(fqdn, cid)
    assert host.fqdn() == fqdn
    assert host.cid() == cid
    services = host.services()
    assert len(services) == 1
    service = services[0]
    assert_that(
        service,
        has_properties(
            {
                'fqdn': fqdn,
                'cid': cid,
                'status': ServiceStatus.alive,
                'role': ServiceRole.master,
                'replicaType': ServiceReplicaType.unknown,
            }
        ),
    )

    cpu = host.system().cpu
    assert_that(
        cpu,
        has_properties(
            {
                'timestamp': system_ts,
                'used': system_cpu_used,
            }
        ),
    )
    memory = host.system().memory
    assert_that(
        memory,
        has_properties(
            {
                'timestamp': system_ts,
                'used': system_memory_used,
                'total': system_memory_total,
            }
        ),
    )
    disk = host.system().disk
    assert_that(
        disk,
        has_properties(
            {
                'timestamp': system_ts,
                'used': system_disk_used,
                'total': system_disk_total,
            }
        ),
    )


def test_mdb_health_client_replica(mocker):
    fqdn = 'fqdn'
    cid = 'cid'
    logger = 'fake'
    system_ts = 1000
    system_cpu_used = 0.1
    system_memory_used = 100
    system_memory_total = 1000
    system_disk_used = 2000
    system_disk_total = 30000
    expected = HealthInfo(
        [
            {
                'fqdn': fqdn,
                'cid': cid,
                'services': [
                    {
                        'name': 'name',
                        'status': 'Alive',
                        'role': 'Replica',
                        'replicatype': 'Async',
                    }
                ],
                'system': {
                    'cpu': {
                        'timestamp': system_ts,
                        'used': system_cpu_used,
                    },
                    'mem': {
                        'timestamp': system_ts,
                        'used': system_memory_used,
                        'total': system_memory_total,
                    },
                    'disk': {
                        'timestamp': system_ts,
                        'used': system_disk_used,
                        'total': system_disk_total,
                    },
                },
            },
        ],
    )

    mock_mdb_health_from_memory(mocker, expected)

    mdbh = MDBHealth()
    mdbh.init_mdbhealth(
        MDBHealthProviderHTTP({'url': 'unknown', 'connect_timeout': 0, 'read_timeout': 0, 'ca_certs': 'nocert'}),
        logger,
    )

    actual = mdbh.health_by_fqdns([fqdn])
    assert actual == expected
    host = actual.host(fqdn, cid)
    assert host.fqdn() == fqdn
    assert host.cid() == cid
    services = host.services()
    assert len(services) == 1
    service = services[0]
    assert_that(
        service,
        has_properties(
            {
                'fqdn': fqdn,
                'cid': cid,
                'status': ServiceStatus.alive,
                'role': ServiceRole.replica,
                'replicaType': ServiceReplicaType.a_sync,
            }
        ),
    )

    cpu = host.system().cpu
    assert_that(
        cpu,
        has_properties(
            {
                'timestamp': system_ts,
                'used': system_cpu_used,
            }
        ),
    )
    memory = host.system().memory
    assert_that(
        memory,
        has_properties(
            {
                'timestamp': system_ts,
                'used': system_memory_used,
                'total': system_memory_total,
            }
        ),
    )
    disk = host.system().disk
    assert_that(
        disk,
        has_properties(
            {
                'timestamp': system_ts,
                'used': system_disk_used,
                'total': system_disk_total,
            }
        ),
    )


@pytest.mark.parametrize(
    'hosts',
    populate_host_healths(valid_hosts, valid_services, valid_system_metrics_without_field),
)
def test_system_metrics_without_fields(mocker, hosts):
    hh = HostHealth(hosts)
    assert hh.system() is not None
    assert hh.system().cpu is not None
    assert hh.system().memory is None
    assert hh.system().disk is not None


@pytest.mark.parametrize(
    'hosts',
    populate_host_healths(valid_hosts, valid_services, empty_system_metrics),
)
def test_empty_system_metrics(mocker, hosts):
    hh = HostHealth(hosts)
    assert hh.system() is None
