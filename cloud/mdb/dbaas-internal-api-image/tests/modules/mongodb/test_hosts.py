"""
Test for Mingodb Hosts
"""

import pytest

from dbaas_internal_api.health.health import HealthInfo, ServiceRole, ServiceStatus
from dbaas_internal_api.modules.mongodb.hosts import diagnose_host, host_role_from_service_role, service_type_from_mdbh
from dbaas_internal_api.modules.mongodb.traits import HostRole, MongoDBRoles, ServiceType
from dbaas_internal_api.utils.types import HostStatus
from ...mocks import mock_logs


def test_host_role_from_service_role():
    assert host_role_from_service_role(ServiceRole.unknown) == HostRole.unknown
    assert host_role_from_service_role(ServiceRole.master) == HostRole.primary
    assert host_role_from_service_role(ServiceRole.replica) == HostRole.secondary


def test_service_type_from_mdbh():
    assert service_type_from_mdbh('mongod') == ServiceType.mongod
    assert service_type_from_mdbh('mongos') == ServiceType.mongos
    assert service_type_from_mdbh('mongocfg') == ServiceType.mongocfg
    assert service_type_from_mdbh('SomeRandomStuff') == ServiceType.unspecified
    assert service_type_from_mdbh('') == ServiceType.unspecified


diagnose_host_input = [
    {
        # Everything is ok, master
        'cid': 'ok',
        'host': {
            'name': 'fqdn',
            'type': MongoDBRoles.mongod,
        },
        'healthinfo': HealthInfo(
            [
                {
                    'cid': 'ok',
                    'fqdn': 'fqdn',
                    'status': 'Alive',
                    'services': [
                        {
                            'name': 'mongod',
                            'status': 'Alive',
                            'role': 'Master',
                        },
                        {
                            'name': 'mongos',
                            'status': 'Alive',
                        },
                        {
                            'name': 'mongocfg',
                            'status': 'Alive',
                        },
                    ],
                    'system': {
                        'cpu': {
                            'timestamp': 1928273,
                            'used': 0.13,
                        },
                        'mem': {
                            'timestamp': 1928273,
                            'used': 100000,
                            'total': 1000000,
                        },
                        'disk': {
                            'timestamp': 1928394,
                            'used': 3000000,
                            'total': 400000000,
                        },
                    },
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'type': MongoDBRoles.mongod,
            'role': HostRole.primary,
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.mongod,
                    'health': ServiceStatus.alive,
                },
                {
                    'type': ServiceType.mongos,
                    'health': ServiceStatus.alive,
                },
                {
                    'type': ServiceType.mongocfg,
                    'health': ServiceStatus.alive,
                },
            ],
            'system': {
                'cpu': {
                    'timestamp': 1928273,
                    'used': 0.13,
                },
                'memory': {
                    'timestamp': 1928273,
                    'used': 100000,
                    'total': 1000000,
                },
                'disk': {
                    'timestamp': 1928394,
                    'used': 3000000,
                    'total': 400000000,
                },
            },
        },
    },
    {
        # Everything is ok, replica
        'cid': 'ok',
        'host': {
            'name': 'fqdn',
            'type': MongoDBRoles.mongod,
        },
        'healthinfo': HealthInfo(
            [
                {
                    'cid': 'ok',
                    'fqdn': 'fqdn',
                    'status': 'Alive',
                    'services': [
                        {
                            'name': 'mongod',
                            'status': 'Alive',
                            'role': 'Replica',
                        },
                    ],
                    'system': {
                        'cpu': {
                            'timestamp': 1928273,
                            'used': 0.13,
                        },
                        'mem': {
                            'timestamp': 1928273,
                            'used': 100000,
                            'total': 1000000,
                        },
                        'disk': {
                            'timestamp': 1928394,
                            'used': 3000000,
                            'total': 400000000,
                        },
                    },
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'type': MongoDBRoles.mongod,
            'role': HostRole.secondary,
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.mongod,
                    'health': ServiceStatus.alive,
                },
            ],
            'system': {
                'cpu': {
                    'timestamp': 1928273,
                    'used': 0.13,
                },
                'memory': {
                    'timestamp': 1928273,
                    'used': 100000,
                    'total': 1000000,
                },
                'disk': {
                    'timestamp': 1928394,
                    'used': 3000000,
                    'total': 400000000,
                },
            },
        },
    },
    {
        # Everything is ok, without system metrics
        'cid': 'ok',
        'host': {
            'name': 'fqdn',
            'type': MongoDBRoles.mongod,
        },
        'healthinfo': HealthInfo(
            [
                {
                    'cid': 'ok',
                    'fqdn': 'fqdn',
                    'status': 'Alive',
                    'services': [
                        {
                            'name': 'mongod',
                            'status': 'Alive',
                            'role': 'Replica',
                        },
                    ],
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'type': MongoDBRoles.mongod,
            'role': HostRole.secondary,
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.mongod,
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
            'type': MongoDBRoles.mongod,
        },
        'healthinfo': HealthInfo(
            [
                {
                    'cid': 'wrong-mdbh-cid',
                    'fqdn': 'fqdn',
                    'services': [
                        {
                            'name': 'mongod',
                            'status': 'Alive',
                            'role': 'Master',
                        },
                        {
                            'name': 'mongos',
                            'status': 'Alive',
                        },
                        {
                            'name': 'mongocfg',
                            'status': 'Alive',
                        },
                    ],
                    'system': {
                        'cpu': {
                            'timestamp': 1928273,
                            'used': 0.13,
                        },
                        'mem': {
                            'timestamp': 1928273,
                            'used': 100000,
                            'total': 1000000,
                        },
                        'disk': {
                            'timestamp': 1928394,
                            'used': 3000000,
                            'total': 400000000,
                        },
                    },
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'type': MongoDBRoles.mongod,
            'role': HostRole.unknown,
            'health': HostStatus.unknown,
            'services': [],
        },
    },
    {
        # Nothing from MDB Health
        'cid': 'nothing',
        'host': {
            'name': 'fqdn',
            'type': MongoDBRoles.mongod,
        },
        'healthinfo': HealthInfo([]),
        'expected': {
            'name': 'fqdn',
            'type': MongoDBRoles.mongod,
            'role': HostRole.unknown,
            'health': HostStatus.unknown,
            'services': [],
        },
    },
]


@pytest.mark.parametrize('input', diagnose_host_input)
def test_diagnose_host(mocker, input):
    mock_logs(mocker)

    assert (
        diagnose_host(input['cid'], input['host']['name'], input['host']['type'], input['healthinfo'])
        == input['expected']
    )
