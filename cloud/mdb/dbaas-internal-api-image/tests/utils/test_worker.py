"""
Tests for worker utilities
"""

from dbaas_internal_api.utils.worker import format_zk_hosts


def test_format_zk_hosts():
    """
    Add port and return csv
    """
    assert format_zk_hosts(['node1', 'node2', 'node3']) == 'node1:2181,node2:2181,node3:2181'
