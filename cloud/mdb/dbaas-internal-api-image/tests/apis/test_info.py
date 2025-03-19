"""
Tests for cluster info
"""

from enum import Enum, unique

import pytest

from dbaas_internal_api.apis.info import deduce_cluster_health
from dbaas_internal_api.utils.types import ClusterStatus


@unique
class HostHealth(Enum):
    """
    Possible host health.
    """

    unknown = 'HostHealthUnknown'
    alive = 'HostHealthAlive'
    dead = 'HostHealthDead'
    degraded = 'HostHealthDegraded'


@unique
class ClusterHealth(Enum):
    """
    Possible cluster health.
    """

    unknown = 'ClusterHealthUnknown'
    alive = 'ClusterHealthAlive'
    dead = 'ClusterHealthDead'
    degraded = 'ClusterHealthDegraded'


@pytest.mark.parametrize(
    ['hosts', 'status', 'res'],
    [
        (
            [{'health': HostHealth.alive}, {'health': HostHealth.alive}, {'health': HostHealth.alive}],
            ClusterStatus.running,
            ClusterHealth.alive,
        ),
        (
            [{'health': HostHealth.unknown}, {'health': HostHealth.unknown}, {'health': HostHealth.unknown}],
            ClusterStatus.running,
            ClusterHealth.unknown,
        ),
        (
            [{'health': HostHealth.dead}, {'health': HostHealth.dead}, {'health': HostHealth.dead}],
            ClusterStatus.running,
            ClusterHealth.dead,
        ),
        (
            [{'health': HostHealth.degraded}, {'health': HostHealth.degraded}, {'health': HostHealth.degraded}],
            ClusterStatus.running,
            ClusterHealth.degraded,
        ),
        (
            [
                {'health': HostHealth.alive},
                {'health': HostHealth.degraded},
                {'health': HostHealth.dead},
                {'health': HostHealth.unknown},
            ],
            ClusterStatus.running,
            ClusterHealth.degraded,
        ),
        (
            [
                {'health': HostHealth.alive},
                {'health': HostHealth.degraded},
                {'health': HostHealth.dead},
                {'health': HostHealth.unknown},
            ],
            ClusterStatus.creating,
            ClusterHealth.unknown,
        ),  # Test for not running cluster
        ([], ClusterStatus.running, ClusterHealth.unknown),  # Test for zero hosts
        ([{'health': HostHealth.alive}], ClusterStatus.running, ClusterHealth.alive),  # Test for one host
        (
            [{'health': HostHealth.alive}],
            ClusterStatus.modifying,
            ClusterHealth.alive,
        ),  # Test for when cluster is being modified (for ex. backup)
    ],
)
def test_deduce_cluster_health(mocker, hosts, status, res):
    assert deduce_cluster_health(hosts, status, HostHealth, ClusterHealth) == res
