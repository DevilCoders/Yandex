"""
Test for dataproc utils
"""

import pytest

from dbaas_internal_api.modules.hadoop.utils import guess_subnet_id
from dbaas_internal_api.core.exceptions import DbaasClientError
from cloud.mdb.internal.python.compute.vpc.api import NetworkNotFoundError
from ...providers import DummyYCNetworkProvider


def raise_network_not_found():
    raise NetworkNotFoundError()


def test_guess_subnet_id():
    vpc = DummyYCNetworkProvider()

    vpc.get_subnets = lambda *_: {'zone-a': {'subnet1': 'folder1'}}
    assert 'subnet1' == guess_subnet_id(vpc, zone_id='zone-a', folder_id='folder1')

    vpc.get_subnets = lambda *_: {'zone-a': {'subnet1': 'folder1', 'subnet2': 'folder1'}}
    expected_exception = (
        'There are more than one subnets in zone zone-a of folder folder1. Please specify which subnet to use.'
    )
    with pytest.raises(DbaasClientError) as excinfo:
        guess_subnet_id(vpc, zone_id='zone-a', folder_id='folder1')
    assert expected_exception in str(excinfo.value)

    vpc.get_subnets = lambda *_: {'zone-a': {}}
    expected_exception = 'There are no subnets in zone zone-a in folder folder1. Please create one.'
    with pytest.raises(DbaasClientError) as excinfo:
        guess_subnet_id(vpc, zone_id='zone-a', folder_id='folder1')
    assert expected_exception in str(excinfo.value)

    vpc.get_networks = raise_network_not_found
    expected_exception = 'Could not find networks in folder folder1.'
    with pytest.raises(DbaasClientError) as excinfo:
        guess_subnet_id(vpc, zone_id='zone-a', folder_id='folder1')
    assert expected_exception in str(excinfo.value)
