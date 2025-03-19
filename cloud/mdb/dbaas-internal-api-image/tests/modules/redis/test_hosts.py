"""
Test for Redis Hosts
"""

import pytest

from dbaas_internal_api.health.health import HealthInfo, ServiceRole, ServiceStatus
from dbaas_internal_api.modules.redis.hosts import diagnose_host, host_role_from_service_role, service_type_from_mdbh
from dbaas_internal_api.modules.redis.traits import HostRole, ServiceType
from dbaas_internal_api.utils.types import HostStatus
from ...mocks import mock_logs


def test_host_role_from_service_role():
    assert host_role_from_service_role(ServiceRole.unknown) == HostRole.unknown
    assert host_role_from_service_role(ServiceRole.master) == HostRole.master
    assert host_role_from_service_role(ServiceRole.replica) == HostRole.replica


def test_service_type_from_mdbh():
    assert service_type_from_mdbh('redis') == ServiceType.redis
    assert service_type_from_mdbh('sentinel') == ServiceType.sentinel
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
                            'name': 'redis',
                            'status': 'Alive',
                            'role': 'Master',
                        },
                        {
                            'name': 'sentinel',
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
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.redis,
                    'health': ServiceStatus.alive,
                },
                {
                    'type': ServiceType.sentinel,
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
        # Empty system metrics
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
                            'name': 'redis',
                            'status': 'Alive',
                            'role': 'Master',
                        },
                        {
                            'name': 'sentinel',
                            'status': 'Alive',
                        },
                    ],
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'role': HostRole.master,
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.redis,
                    'health': ServiceStatus.alive,
                },
                {
                    'type': ServiceType.sentinel,
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
                            'name': 'redis',
                            'status': 'Alive',
                            'role': 'Master',
                        },
                        {
                            'name': 'sentinel',
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
            'health': HostStatus.unknown,
            'services': [],
        },
    },
]


@pytest.mark.parametrize('input', diagnose_host_input)
def test_diagnose_host(mocker, input):
    mock_logs(mocker)

    assert diagnose_host(input['cid'], input['host']['name'], input['healthinfo']) == input['expected']
