"""
Test for ClickHouse Hosts
"""

import pytest

from dbaas_internal_api.health.health import HealthInfo, ServiceStatus
from dbaas_internal_api.modules.clickhouse.hosts import diagnose_host, service_type_from_mdbh
from dbaas_internal_api.modules.clickhouse.traits import ServiceType
from dbaas_internal_api.utils.types import HostStatus
from ...mocks import mock_logs


def test_service_type_from_mdbh():
    assert service_type_from_mdbh('clickhouse') == ServiceType.clickhouse
    assert service_type_from_mdbh('zookeeper') == ServiceType.zookeeper
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
                            'name': 'clickhouse',
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
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.clickhouse,
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
                            'name': 'clickhouse',
                            'status': 'Alive',
                        },
                    ],
                }
            ]
        ),
        'expected': {
            'name': 'fqdn',
            'health': HostStatus.alive,
            'services': [
                {
                    'type': ServiceType.clickhouse,
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
                    'status': 'Unknown',
                    'services': [
                        {
                            'name': 'clickhouse',
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
            'health': HostStatus.unknown,
            'services': [],
        },
    },
]


@pytest.mark.parametrize('input', diagnose_host_input)
def test_diagnose_host(mocker, input):
    mock_logs(mocker)

    assert diagnose_host(input['cid'], input['host']['name'], input['healthinfo']) == input['expected']
