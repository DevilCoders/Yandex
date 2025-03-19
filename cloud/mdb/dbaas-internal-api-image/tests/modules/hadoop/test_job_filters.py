"""
Test for Hadoop job filters
"""

import pytest

from dbaas_internal_api.core.exceptions import DbaasNotImplementedError
from dbaas_internal_api.modules.hadoop.jobs import parse_filters
from dbaas_internal_api.utils.filters_parser import parse


@pytest.mark.parametrize(
    ['filters_str'],
    [
        ("status > 'provisioning'",),
        ("status < 'provisioning'",),
        ("status <= 'provisioning'",),
        ("status >= 'provisioning'",),
        ("status = 'test'",),
        ("status = 'running' AND status = 'provisioning'",),
        ("test = 'running'",),
    ],
)
def test_not_supported_filter(filters_str):
    filters = parse(filters_str)
    with pytest.raises(DbaasNotImplementedError):
        parse_filters(filters)


@pytest.mark.parametrize(
    ['filters_str', 'result'],
    [
        ("status = 'running'", ['RUNNING']),
        ("status = 'ERROR'", ['ERROR']),
        ("status IN ('provisioning', 'DONE')", ['PROVISIONING', 'DONE']),
        ("status = 'active'", ['PROVISIONING', 'PENDING', 'RUNNING', 'CANCELLING']),
        ("status NOT IN ('active', 'provisioning', 'done')", ['ERROR', 'CANCELLED']),
    ],
)
def test_correct_filter(filters_str, result):
    filters = parse(filters_str)
    assert sorted(parse_filters(filters)) == sorted(result)
