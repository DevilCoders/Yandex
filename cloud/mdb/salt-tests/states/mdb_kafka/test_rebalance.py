# coding: utf-8

from __future__ import print_function
from cloud.mdb.salt.salt._states import mdb_kafka


def replicas_from_nodes(nodes):
    replicas = {}
    for node in nodes.values():
        for partition in node.get('partitions', []):
            if partition not in replicas:
                replicas[partition] = []
            replicas[partition].append(node['id'])
    return replicas


def get_racks_imbalance(state):
    racks_partitions = [len(rack['partitions']) for rack in state.racks.values()]
    return max(racks_partitions) - min(racks_partitions)


def get_broker_imbalance(state):
    brokers_partitions = [len(broker['partitions']) for broker in state.brokers.values()]
    return max(brokers_partitions) - min(brokers_partitions)


def get_racks_leaders_imbalance(state):
    leaders_info, _ = state.get_leaders_info()
    leaders_racks = [len(leaders) for leaders in leaders_info.values()]
    return max(leaders_racks) - min(leaders_racks)


def get_brokers_leaders_imbalance(state):
    _, brokers_info = state.get_leaders_info()
    leaders_brokers = [len(leaders) for leaders in brokers_info.values()]
    return max(leaders_brokers) - min(leaders_brokers)


def test_add_rack():
    nodes = {
        'rc1a-1': {'id': 1, 'rack': 'ru-central1-a', 'partitions': {0, 2, 4, 6, 8, 10}},
        'rc1b-1': {'id': 2, 'rack': 'ru-central1-b', 'partitions': {1, 3, 5, 7, 9, 11}},
        'rc1c-1': {'id': 3, 'rack': 'ru-central1-c'},
    }

    state = mdb_kafka.ReplicasState(nodes, replicas_from_nodes(nodes))
    state.balance()
    assert get_racks_imbalance(state) == 0, 'Racks partitions imbalance ' + str(state.racks)
    assert get_broker_imbalance(state) == 0, 'Broker partitions imbalance ' + str(state.brokers)
    assert get_racks_leaders_imbalance(state) == 0, 'Racks leaders imbalance ' + str(state.get_leaders_info())
    assert get_brokers_leaders_imbalance(state) == 0, 'Brokers leaders imbalance ' + str(state.get_leaders_info())


def test_add_rack_imbalanced():
    nodes = {
        'rc1a-1': {'id': 1, 'rack': 'ru-central1-a', 'partitions': {0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
        'rc1b-1': {'id': 2, 'rack': 'ru-central1-b', 'partitions': {1}},
        'rc1c-1': {'id': 3, 'rack': 'ru-central1-c'},
    }

    state = mdb_kafka.ReplicasState(nodes, replicas_from_nodes(nodes))
    state.balance()
    assert get_racks_imbalance(state) == 0, 'Racks partitions imbalance ' + str(state.racks)
    assert get_broker_imbalance(state) == 0, 'Broker partitions imbalance ' + str(state.brokers)
    assert get_racks_leaders_imbalance(state) == 0, 'Racks leaders imbalance ' + str(state.get_leaders_info())
    assert get_brokers_leaders_imbalance(state) == 0, 'Brokers leaders imbalance ' + str(state.get_leaders_info())


def test_six_brokers_imbalanced():
    nodes = {
        'rc1a-1': {'id': 1, 'rack': 'ru-central1-a', 'partitions': {0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
        'rc1a-2': {'id': 2, 'rack': 'ru-central1-a'},
        'rc1b-1': {'id': 3, 'rack': 'ru-central1-b', 'partitions': {1}},
        'rc1b-2': {'id': 4, 'rack': 'ru-central1-b'},
        'rc1c-1': {'id': 5, 'rack': 'ru-central1-c'},
        'rc1c-2': {'id': 6, 'rack': 'ru-central1-c'},
    }

    state = mdb_kafka.ReplicasState(nodes, replicas_from_nodes(nodes))
    state.balance()
    assert get_racks_imbalance(state) == 0, 'Racks partitions imbalance ' + str(state.racks)
    assert get_broker_imbalance(state) == 0, 'Broker partitions imbalance ' + str(state.brokers)
    assert get_racks_leaders_imbalance(state) == 0, 'Racks leaders imbalance ' + str(state.get_leaders_info())
    assert get_brokers_leaders_imbalance(state) == 0, 'Brokers leaders imbalance ' + str(state.get_leaders_info())


def test_six_brokers_two_replicas():
    nodes = {
        'rc1a-1': {'id': 1, 'rack': 'ru-central1-a', 'partitions': {0, 2, 4, 6, 8, 10}},
        'rc1a-2': {'id': 2, 'rack': 'ru-central1-a', 'partitions': {1, 3, 5, 7, 9, 11}},
        'rc1b-1': {'id': 3, 'rack': 'ru-central1-b', 'partitions': {0, 2, 4, 6, 8, 10}},
        'rc1b-2': {'id': 4, 'rack': 'ru-central1-b', 'partitions': {1, 3, 5, 7, 9, 11}},
        'rc1c-1': {'id': 5, 'rack': 'ru-central1-c'},
        'rc1c-2': {'id': 6, 'rack': 'ru-central1-c'},
    }

    state = mdb_kafka.ReplicasState(nodes, replicas_from_nodes(nodes))
    state.balance()
    assert get_racks_imbalance(state) == 0, 'Racks partitions imbalance ' + str(state.racks)
    assert get_broker_imbalance(state) == 0, 'Broker partitions imbalance ' + str(state.brokers)
    assert get_racks_leaders_imbalance(state) == 0, 'Racks leaders imbalance ' + str(state.get_leaders_info())
    assert get_brokers_leaders_imbalance(state) == 0, 'Brokers leaders imbalance ' + str(state.get_leaders_info())


def test_fix_replication_racks():
    nodes = {
        'rc1a-1': {'id': 1, 'rack': 'ru-central1-a', 'partitions': {0, 2, 4, 6, 8, 10}},
        'rc1a-2': {'id': 2, 'rack': 'ru-central1-a', 'partitions': {1, 3, 5, 7, 9, 12}},
        'rc1b-1': {'id': 3, 'rack': 'ru-central1-b', 'partitions': {0, 2, 4, 6, 8, 10}},
        'rc1b-2': {'id': 4, 'rack': 'ru-central1-b', 'partitions': {1, 3, 5, 7, 9, 11}},
        'rc1c-1': {'id': 5, 'rack': 'ru-central1-c', 'partitions': {13, 11}},
        'rc1c-2': {'id': 6, 'rack': 'ru-central1-c', 'partitions': {12, 13}},
    }

    state = mdb_kafka.ReplicasState(nodes, replicas_from_nodes(nodes))
    state.balance()
    assert get_racks_imbalance(state) == 1, 'Racks partitions imbalance ' + str(state.racks)
    assert get_broker_imbalance(state) == 1, 'Broker partitions imbalance ' + str(state.brokers)
    assert get_racks_leaders_imbalance(state) == 1, 'Racks leaders imbalance ' + str(state.get_leaders_info())
    assert get_brokers_leaders_imbalance(state) == 1, 'Brokers leaders imbalance ' + str(state.get_leaders_info())


def test_one_rack():
    nodes = {
        'rc1a-1': {'id': 1, 'rack': 'ru-central1-a', 'partitions': {0, 1, 2, 3, 4, 5, 6}},
        'rc1a-2': {'id': 2, 'rack': 'ru-central1-a', 'partitions': {0, 1, 2, 3, 4, 5, 6}},
        'rc1a-3': {'id': 3, 'rack': 'ru-central1-a'},
    }

    state = mdb_kafka.ReplicasState(nodes, replicas_from_nodes(nodes))
    state.balance()
    assert get_broker_imbalance(state) == 1, 'Broker partitions imbalance ' + str(state.brokers)
    assert get_brokers_leaders_imbalance(state) == 1, 'Brokers leaders imbalance ' + str(state.get_leaders_info())


def test_not_balance_racks():
    nodes = {
        'rc1a-1': {'id': 1, 'rack': 'ru-central1-a', 'partitions': {0, 1, 2, 3, 4, 5, 6}},
        'rc1a-2': {'id': 2, 'rack': 'ru-central1-a', 'partitions': {0, 1, 2, 3, 4, 5, 6}},
        'rc1a-3': {'id': 3, 'rack': 'ru-central1-a', 'partitions': {0, 1, 2, 3, 4, 5, 6}},
        'rc1b-1': {'id': 4, 'rack': 'ru-central1-b'},
        'rc1b-2': {'id': 5, 'rack': 'ru-central1-b'},
        'rc1b-3': {'id': 6, 'rack': 'ru-central1-b'},
    }

    state = mdb_kafka.ReplicasState(nodes, replicas_from_nodes(nodes))
    state.balance()
    assert get_broker_imbalance(state) == 1, 'Broker partitions imbalance ' + str(state.brokers)
    assert get_brokers_leaders_imbalance(state) == 1, 'Brokers leaders imbalance ' + str(state.get_leaders_info())


def test_infinite_loop_prevention():
    nodes = {
        'rc1a-1': {'id': 1, 'rack': 'ru-central1-a'},
        'rc1a-2': {'id': 2, 'rack': 'ru-central1-a'},
        'rc1a-3': {'id': 3, 'rack': 'ru-central1-a'},
    }
    replicas = {7: [1, 3], 6: [1, 3], 5: [2, 3], 4: [2, 3], 3: [2, 1], 2: [1, 2], 1: [2, 1]}

    state = mdb_kafka.ReplicasState(nodes, replicas)
    state.balance()
    assert get_broker_imbalance(state) == 1, 'Broker partitions imbalance ' + str(state.brokers)
    assert get_brokers_leaders_imbalance(state) == 1, 'Brokers leaders imbalance ' + str(state.get_leaders_info())


def test_removed_hosts():
    nodes = {
        'az1-1': {'id': 1, 'rack': 'az1', 'partitions': {0}},
        'az1-2': {'id': 2, 'rack': 'az1', 'partitions': {1}},
        'az2-1': {'id': 3, 'rack': 'az2', 'partitions': {0}},
        'az2-2': {'id': 4, 'rack': 'az2', 'partitions': {1}},
    }
    replicas = {
        0: [1, 3, 5],
        1: [6, 4, 2],
    }

    state = mdb_kafka.ReplicasState(nodes, replicas)
    state.balance()

    assert len(state.topic_replicas[0]) == 3
    assert len(state.topic_replicas[1]) == 3
