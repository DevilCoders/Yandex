"""
Tests for Redis tasks-related utils.
"""
from hamcrest import assert_that, equal_to, has_length

from cloud.mdb.dbaas_worker.internal.tasks.redis.utils import separate_slaves_and_masters, get_one_by_one_order

# pylint: disable=invalid-name, missing-docstring


def _sort(lst):
    return sorted(lst, key=lambda x: list(x.keys())[0])


def check_separate_slaves_and_masters(masters_dict, slaves_dict):
    hosts = dict(slaves_dict)
    hosts.update(masters_dict)

    separated = separate_slaves_and_masters(hosts, list(masters_dict.keys()))

    check_masters(separated, masters_dict)

    # check slaves
    masters_num = len(masters_dict)
    separated_slaves_list = separated[:-masters_num]
    assert_that(separated_slaves_list, has_length(1), "slaves_list={}".format(separated_slaves_list))
    assert_that(separated_slaves_list[0], equal_to(slaves_dict))


def get_single_data():
    masters_dict = {'m1': {'shard_id': 'shard1'}}
    slaves_dict = {f's{i}': {'shard_id': 'shard1'} for i in range(6)}
    return masters_dict, slaves_dict


def get_sharded_data():
    masters_dict = {name: {'shard_id': f'shard{i}'} for i, name in enumerate(['m1', 'm2', 'm3'])}
    slaves_dict = {
        's1': {'shard_id': 'shard1'},
        's2': {'shard_id': 'shard1'},
        's3': {'shard_id': 'shard2'},
        's4': {'shard_id': 'shard3'},
    }
    return masters_dict, slaves_dict


def check_masters(dict_list, masters_dict):
    masters_num = len(masters_dict)
    separated_masters = _sort(dict_list[-masters_num:])
    flatten_masters = _sort({k: v} for k, v in masters_dict.items())
    assert_that(
        separated_masters, equal_to(flatten_masters), "res={}; exp={}".format(separated_masters, flatten_masters)
    )


def test_separate_slaves_and_masters_sharded():
    masters_dict, slaves_dict = get_sharded_data()
    check_separate_slaves_and_masters(masters_dict, slaves_dict)


def test_separate_slaves_and_masters_single():
    masters_dict, slaves_dict = get_single_data()
    check_separate_slaves_and_masters(masters_dict, slaves_dict)


def test_get_one_by_one_order():
    hosts = {'s1': None, 's5': None, 's3': None, 'm5': None, 'm2': None}
    master_list = ['m2', 'm5']
    expected = ['s1', 's5', 's3', 'm5', 'm2']
    actual = get_one_by_one_order(hosts, master_list)
    assert_that(actual, equal_to(expected))

    hosts = {'s1': None, 's5': None, 's3': None, 'm5': None, 'm2': None}
    master_list = []
    expected = ['s1', 's5', 's3', 'm5', 'm2']
    actual = get_one_by_one_order(hosts, master_list)
    assert_that(actual, equal_to(expected))

    hosts = {}
    master_list = []
    expected = []
    actual = get_one_by_one_order(hosts, master_list)
    assert_that(actual, equal_to(expected))
