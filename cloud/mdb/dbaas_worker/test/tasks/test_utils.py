"""
Tests for tasks-related utils.
"""
from collections import ChainMap

from hamcrest import assert_that, contains, has_entries, has_length

from cloud.mdb.dbaas_worker.internal.tasks.utils import split_hosts_for_modify_by_shard_batches

# pylint: disable=invalid-name, missing-docstring


def test_split_hosts_for_modify_by_shard_batches():
    hosts = _generate_host_set(4)
    host_groups = split_hosts_for_modify_by_shard_batches(hosts)
    assert_that(host_groups, contains(has_length(1), has_length(2), has_length(1)))
    assert_that(ChainMap(*host_groups), has_entries(**hosts))


def test_split_hosts_for_modify_by_shard_batches_with_limit():
    hosts = _generate_host_set(11)
    host_groups = split_hosts_for_modify_by_shard_batches(hosts)
    assert_that(host_groups, contains(has_length(1), has_length(2), has_length(4), has_length(4)))
    assert_that(ChainMap(*host_groups), has_entries(**hosts))
    host_groups = split_hosts_for_modify_by_shard_batches(hosts, max_batch_size=3)
    assert_that(host_groups, contains(has_length(1), has_length(2), has_length(3), has_length(3), has_length(2)))
    assert_that(ChainMap(*host_groups), has_entries(**hosts))


def test_split_hosts_for_modify_by_shard_batches_fast_mode():
    hosts = _generate_host_set(4)
    host_groups = split_hosts_for_modify_by_shard_batches(hosts, fast_mode=True)
    assert_that(host_groups, contains(has_length(4)))
    assert_that(ChainMap(*host_groups), has_entries(**hosts))


def _generate_host_set(size):
    hosts = {}
    for i in range(1, size + 1):
        hosts[f'man-{i}'] = {'shard_id': f'shard{i}'}
    return hosts
