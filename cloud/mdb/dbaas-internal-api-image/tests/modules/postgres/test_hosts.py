"""
Test for PG Hosts
"""

import pytest

from dbaas_internal_api.health.health import HealthInfo, ServiceReplicaType, ServiceRole, ServiceStatus
from dbaas_internal_api.modules.postgres.hosts import (
    diagnose_host,
    host_role_from_service_role,
    replica_type_from_service_to_host,
    service_type_from_mdbh,
)
from dbaas_internal_api.modules.postgres.traits import HostReplicaType, HostRole, ServiceType
from dbaas_internal_api.utils.types import HostStatus
from ...mocks import mock_logs


def test_host_role_from_service_role():
    assert host_role_from_service_role(ServiceRole.unknown) == HostRole.unknown
    assert host_role_from_service_role(ServiceRole.master) == HostRole.master
    assert host_role_from_service_role(ServiceRole.replica) == HostRole.replica


def test_replica_type_from_service_to_host():
    assert replica_type_from_service_to_host(ServiceReplicaType.unknown) == HostReplicaType.unknown
    assert replica_type_from_service_to_host(ServiceReplicaType.a_sync) == HostReplicaType.a_sync
    assert replica_type_from_service_to_host(ServiceReplicaType.sync) == HostReplicaType.sync


def test_service_type_from_mdbh():
    assert service_type_from_mdbh('pg_replication') == ServiceType.postgres
    assert service_type_from_mdbh('pgbouncer') == ServiceType.pooler
    assert service_type_from_mdbh('SomeRandomStuff') == ServiceType.unspecified
    assert service_type_from_mdbh('') == ServiceType.unspecified


diagnose_host_input = [
    {
        # Everything is ok
        'cid': 'ok',
        'host': {
            'name': 'fqdn',
        },
        'healthinfo': HealthInfo(
            [
                {
                    'cid': 'ok',
                    'fqdn': 'fqdn',
                    'status': 'Alive',
                    'services': [
                        {
                            'name': 'pg_replication',
                            'status': 'Alive',
                            'role': 'Master',
                            'replicatype': 'Unknown',
                        },
                        {
                            'name': 'pgbouncer',
                            'status': 'Alive',
                        },
                    ],
                    'system': {
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
                    },
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'role': HostRole.master,
            'replica_type': HostReplicaType.unknown,
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.postgres,
                    'health': ServiceStatus.alive,
                },
                {
                    'type': ServiceType.pooler,
                    'health': ServiceStatus.alive,
                },
            ],
            'system': {
                'cpu': {
                    'timestamp': 1000,
                    'used': 0.1,
                },
                'memory': {
                    'timestamp': 1000,
                    'used': 100,
                    'total': 1000,
                },
                'disk': {
                    'timestamp': 1000,
                    'used': 2000,
                    'total': 30000,
                },
            },
        },
    },
    {
        # Replica is ok
        'cid': 'ok',
        'host': {
            'name': 'fqdn',
        },
        'healthinfo': HealthInfo(
            [
                {
                    'cid': 'ok',
                    'fqdn': 'fqdn',
                    'status': 'Alive',
                    'services': [
                        {
                            'name': 'pg_replication',
                            'status': 'Alive',
                            'role': 'Replica',
                            'replicatype': 'Sync',
                        },
                        {
                            'name': 'pgbouncer',
                            'status': 'Alive',
                        },
                    ],
                    'system': {
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
                    },
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'role': HostRole.replica,
            'replica_type': HostReplicaType.sync,
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.postgres,
                    'health': ServiceStatus.alive,
                },
                {
                    'type': ServiceType.pooler,
                    'health': ServiceStatus.alive,
                },
            ],
            'system': {
                'cpu': {
                    'timestamp': 1000,
                    'used': 0.1,
                },
                'memory': {
                    'timestamp': 1000,
                    'used': 100,
                    'total': 1000,
                },
                'disk': {
                    'timestamp': 1000,
                    'used': 2000,
                    'total': 30000,
                },
            },
        },
    },
    {
        # Without field in system metrics
        'cid': 'ok',
        'host': {
            'name': 'fqdn',
        },
        'healthinfo': HealthInfo(
            [
                {
                    'cid': 'ok',
                    'fqdn': 'fqdn',
                    'status': 'Alive',
                    'services': [
                        {
                            'name': 'pg_replication',
                            'status': 'Alive',
                            'role': 'Replica',
                            'replicatype': 'Sync',
                        },
                        {
                            'name': 'pgbouncer',
                            'status': 'Alive',
                        },
                    ],
                    'system': {
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
                    },
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'role': HostRole.replica,
            'replica_type': HostReplicaType.sync,
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.postgres,
                    'health': ServiceStatus.alive,
                },
                {
                    'type': ServiceType.pooler,
                    'health': ServiceStatus.alive,
                },
            ],
            'system': {
                'memory': {
                    'timestamp': 1000,
                    'used': 100,
                    'total': 1000,
                },
                'disk': {
                    'timestamp': 1000,
                    'used': 2000,
                    'total': 30000,
                },
            },
        },
    },
    {
        # Without  system metrics
        'cid': 'ok',
        'host': {
            'name': 'fqdn',
        },
        'healthinfo': HealthInfo(
            [
                {
                    'cid': 'ok',
                    'fqdn': 'fqdn',
                    'status': 'Alive',
                    'services': [
                        {
                            'name': 'pg_replication',
                            'status': 'Alive',
                            'role': 'Replica',
                            'replicatype': 'Sync',
                        },
                        {
                            'name': 'pgbouncer',
                            'status': 'Alive',
                        },
                    ],
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'role': HostRole.replica,
            'replica_type': HostReplicaType.sync,
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.postgres,
                    'health': ServiceStatus.alive,
                },
                {
                    'type': ServiceType.pooler,
                    'health': ServiceStatus.alive,
                },
            ],
        },
    },
    {
        # Wrong cid
        'cid': 'wrong',
        'host': {
            'name': 'fqdn',
        },
        'healthinfo': HealthInfo(
            [
                {
                    'cid': 'wrong-mdbh-cid',
                    'fqdn': 'fqdn',
                    'services': [
                        {
                            'name': 'pg_replication',
                            'status': 'Alive',
                            'role': 'Master',
                            'replicatype': 'Unknown',
                        },
                        {
                            'name': 'pgbouncer',
                            'status': 'Alive',
                        },
                    ],
                    'system': {
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
                    },
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'role': HostRole.unknown,
            'replica_type': HostReplicaType.unknown,
            'health': HostStatus.unknown,
            'services': [],
        },
    },
    {
        # Nothing from MDB Health
        'cid': 'nothing',
        'host': {
            'name': 'fqdn',
        },
        'healthinfo': HealthInfo([]),
        'expected': {
            'name': 'fqdn',
            'role': HostRole.unknown,
            'replica_type': HostReplicaType.unknown,
            'health': HostStatus.unknown,
            'services': [],
        },
    },
]


@pytest.mark.parametrize('input', diagnose_host_input)
def test_diagnose_host(mocker, input):
    mock_logs(mocker)
    assert diagnose_host(input['cid'], input['host']['name'], input['healthinfo']) == input['expected']
