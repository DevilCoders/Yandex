# -*- coding: utf-8 -*-
import json

from cloud.mdb.kafka_agent.internal.topic_sync import config_value_from_string, is_zero, Topic, TopicSync
from yandex.cloud.priv.mdb.kafka.v1.inner import (
    topic_service_pb2 as mdb_topic_service_pb2,
    topic_service_pb2_grpc as mdb_topic_service_pb2_grpc,
)
from datacloud.kafka.inner.v1 import (
    topic_service_pb2 as datacloud_topic_service_pb2,
    topic_service_pb2_grpc as datacloud_topic_service_pb2_grpc,
)


def build_topic_sync(vtype: str = 'compute'):
    config = {
        'cluster_id': 'xxx',
        'vtype': vtype,
        'logging': {},
        'intapi': {
            'url': '',
        },
        'service_account': {
            'id': 'xxx',
            'key_id': 'yyy',
            'private_key': 'zzz',
        },
        'iam_jwt': {
            'url': '',
        },
    }
    topic_sync = TopicSync(config)
    topic_sync.known_topic_config_properties = [
        'flush.ms',
        'flush.messages',
        'min.insync.replicas',
        'preallocate',
        'retention.ms',
    ]
    return topic_sync


def test_compare_topic_changed_partitions():
    kafka_topic = Topic(name='topic1', partitions=24, replication_factor=3, config={})
    pillar_topic = Topic(name='topic1', partitions=12, replication_factor=3, config={})
    changes, topic = build_topic_sync().compare_kafka_and_pillar_topic(kafka_topic, pillar_topic)
    assert changes == 'partitions will be changed from 12 to 24'


def test_compare_topic_changed_replication_factor():
    kafka_topic = Topic(name='topic1', partitions=12, replication_factor=2, config={})
    pillar_topic = Topic(name='topic1', partitions=12, replication_factor=3, config={})
    changes, topic = build_topic_sync().compare_kafka_and_pillar_topic(kafka_topic, pillar_topic)
    assert changes == 'replication_factor will be changed from 3 to 2'


def test_compare_topic_changed_config():
    kafka_topic = Topic(
        name='topic1',
        partitions=12,
        replication_factor=3,
        config={
            'flush.ms': 10000,
            'retention.ms': 10000,
        },
    )
    pillar_topic = Topic(
        name='topic1',
        partitions=12,
        replication_factor=3,
        config={
            'flush.ms': 1000,
            'flush.messages': 1000,
        },
    )
    changes, topic = build_topic_sync().compare_kafka_and_pillar_topic(kafka_topic, pillar_topic)
    assert (
        changes == 'flush.messages will be deleted, flush.ms will be changed from 1000 to 10000,'
        ' retention.ms will be set to 10000'
    )


def test_compare_topic_with_unknown_config_prop():
    kafka_topic = Topic(
        name='topic1',
        partitions=12,
        replication_factor=3,
        config={
            'flush.ms': 1000,
            'message.timestamp.difference.max.ms': 1000,
        },
    )
    pillar_topic = Topic(
        name='topic1',
        partitions=12,
        replication_factor=3,
        config={
            'flush.ms': 1000,
        },
    )
    changes, topic = build_topic_sync().compare_kafka_and_pillar_topic(kafka_topic, pillar_topic)
    assert changes == ''


def test_compare_topic_min_insync_replicas():
    cases = [
        {'replication_factor': 1, 'min_insync_replicas': 1, 'changes': ''},
        {'replication_factor': 2, 'min_insync_replicas': 1, 'changes': ''},
        {'replication_factor': 2, 'min_insync_replicas': 2, 'changes': 'min.insync.replicas will be set to 2'},
        {'replication_factor': 3, 'min_insync_replicas': 1, 'changes': 'min.insync.replicas will be set to 1'},
        {'replication_factor': 3, 'min_insync_replicas': 2, 'changes': ''},
        {'replication_factor': 3, 'min_insync_replicas': 3, 'changes': 'min.insync.replicas will be set to 3'},
        {
            'replication_factor': 3,
            'min_insync_replicas': 2,
            'pillar_min_isr': 1,
            'changes': 'min.insync.replicas will be changed from 1 to 2',
        },
    ]
    for case in cases:
        kafka_topic = Topic(
            name='topic1',
            partitions=12,
            replication_factor=case['replication_factor'],
            config={
                'min.insync.replicas': case['min_insync_replicas'],
            },
        )
        pillar_topic = Topic(name='topic1', partitions=12, replication_factor=case['replication_factor'], config={})
        if 'pillar_min_isr' in case:
            pillar_topic.config['min.insync.replicas'] = case['pillar_min_isr']
        changes, topic = build_topic_sync().compare_kafka_and_pillar_topic(kafka_topic, pillar_topic)
        assert changes == case['changes']
        if changes:
            assert topic.config.get('min.insync.replicas') == case['min_insync_replicas']
        else:
            assert 'min.insync.replicas' not in topic.config


def test_compare_topic_no_defaults():
    kafka_topic = Topic(
        name='topic1',
        partitions=12,
        replication_factor=3,
        config={
            'preallocate': False,
        },
    )
    pillar_topic = Topic(name='topic1', partitions=12, replication_factor=3, config={})
    changes, topic = build_topic_sync().compare_kafka_and_pillar_topic(kafka_topic, pillar_topic)
    assert changes == ''
    assert 'preallocate' not in topic.config


def test_is_zero():
    assert is_zero(0)
    assert is_zero(False)
    assert is_zero("")
    assert not is_zero(1)
    assert not is_zero(True)
    assert not is_zero("hello world")


def test_config_value_from_string():
    assert config_value_from_string('true') is True
    assert config_value_from_string('false') is False
    assert config_value_from_string('1024') == 1024
    assert config_value_from_string('snappy') == 'snappy'


def test_compare_all_topics():
    pillar_topics = {
        'topic1': Topic(name='topic1', partitions=12, replication_factor=2, config={}),
        'topic2': Topic(name='topic2', partitions=12, replication_factor=2, config={'flush.ms': 10000}),
        'topic3': Topic(name='topic3', partitions=12, replication_factor=2, config={}),
    }
    kafka_topics = {
        'topic1': Topic(name='topic1', partitions=12, replication_factor=2, config={}),
        'topic2': Topic(
            name='topic2',
            partitions=12,
            replication_factor=2,
            config={'flush.ms': 20000, 'min.insync.replicas': 1, 'preallocate': False},
        ),
        'topic4': Topic(
            name='topic4',
            partitions=12,
            replication_factor=2,
            config={'flush.ms': 10000, 'min.insync.replicas': 1, 'preallocate': False},
        ),
    }
    changes = build_topic_sync().compare_all_topics(pillar_topics, kafka_topics)
    assert changes['updated'] == [Topic(name='topic2', partitions=12, replication_factor=2, config={'flush.ms': 20000})]
    assert changes['deleted'] == {'topic3'}
    # when topic is added no config values are ignored
    assert changes['new'] == [
        Topic(
            name='topic4',
            partitions=12,
            replication_factor=2,
            config={'flush.ms': 10000, 'min.insync.replicas': 1, 'preallocate': False},
        )
    ]


def test_no_changes_if_only_service_topic_is_added():
    pillar_topics = {
        'topic1': Topic(name='topic1', partitions=12, replication_factor=2, config={}),
    }
    kafka_topics = {
        'topic1': Topic(name='topic1', partitions=12, replication_factor=2, config={}),
        '__consumer_offsets': Topic(name='__consumer_offsets', partitions=50, replication_factor=1, config={}),
    }
    changes = build_topic_sync().compare_all_topics(pillar_topics, kafka_topics)
    assert changes == {}


def test_topic_sync_init_for_vtype_compute():
    topic_sync = build_topic_sync(vtype='compute')
    assert topic_sync.topic_service_pb2 == mdb_topic_service_pb2
    assert topic_sync.topic_service_pb2_grpc == mdb_topic_service_pb2_grpc
    assert isinstance(topic_sync.topic_service.service, mdb_topic_service_pb2_grpc.TopicServiceStub)


def test_topic_sync_init_for_vtype_aws():
    topic_sync = build_topic_sync(vtype='aws')
    assert topic_sync.topic_service_pb2 == datacloud_topic_service_pb2
    assert topic_sync.topic_service_pb2_grpc == datacloud_topic_service_pb2_grpc
    assert isinstance(topic_sync.topic_service.service, datacloud_topic_service_pb2_grpc.TopicServiceStub)


def check_build_update_request(vtype):
    new_topic = Topic(name='new_topic', partitions=12, replication_factor=2, config={'flush.ms': 20000})
    updated_topic = Topic(name='updated_topic', partitions=3, replication_factor=1, config={})
    changes = {'new': [new_topic], 'updated': [updated_topic], 'deleted': {'deleted_topic'}}

    request = build_topic_sync(vtype=vtype).build_update_request(changes, 123)

    assert request.cluster_id == 'xxx'
    assert request.revision == 123
    assert len(request.changed_topics) == 2
    assert [json.loads(i) for i in request.changed_topics] == [
        {'name': 'new_topic', 'partitions': 12, 'replication_factor': 2, 'config': {'flush.ms': 20000}},
        {'name': 'updated_topic', 'partitions': 3, 'replication_factor': 1, 'config': {}},
    ]
    assert request.deleted_topics == ['deleted_topic']

    return request


def test_build_update_request_for_vtype_compute():
    request = check_build_update_request('compute')
    assert isinstance(request, mdb_topic_service_pb2.UpdateTopicsRequest)


def test_build_update_request_for_vtype_aws():
    request = check_build_update_request('aws')
    assert isinstance(request, datacloud_topic_service_pb2.UpdateTopicsRequest)


def mock_topic_service_list(
    topic_service_pb2, update_allowed: bool, revision: int, topics: list, known_topic_config_properties: dict
):
    def mock_list(request):
        assert request.cluster_id == 'xxx'
        return topic_service_pb2.ListTopicsResponse(
            update_allowed=update_allowed,
            topics=topics,
            known_topic_config_properties=known_topic_config_properties,
            revision=revision,
        )

    return mock_list


def test_get_pillar_topics_for_vtype_compute_when_update_is_not_allowed():
    topic_sync = build_topic_sync(vtype='compute')
    topic_sync.topic_service.List = mock_topic_service_list(mdb_topic_service_pb2, False, 123, [], {})

    pillar_topics, revision, success = topic_sync.get_pillar_topics()

    assert not success
    assert pillar_topics == {}
    assert revision == 0


def check_get_pillar_topics_when_update_is_allowed(vtype: str, topic_service_pb2):
    known_topic_config_properties = {'some_config_key': 'some_config_value'}
    topics = [
        '''
        {
            "name": "topic_name1",
            "partitions": 12,
            "replication_factor": 2,
            "config": {
                "topic_config_key": "topic_config_value"
            }
        }
        ''',
        '''
        {
            "name": "topic_name2",
            "partitions": 5,
            "replication_factor": 1,
            "config": {}
        }
        ''',
    ]

    topic_sync = build_topic_sync(vtype=vtype)
    topic_sync.topic_service.List = mock_topic_service_list(
        topic_service_pb2, True, 123, topics, known_topic_config_properties
    )

    pillar_topics, revision, success = topic_sync.get_pillar_topics()

    assert success
    assert pillar_topics == {
        'topic_name1': Topic(
            name='topic_name1',
            partitions=12,
            replication_factor=2,
            config={'topic_config_key': 'topic_config_value'},
        ),
        'topic_name2': Topic(
            name='topic_name2',
            partitions=5,
            replication_factor=1,
            config={},
        ),
    }
    assert revision == 123
    assert topic_sync.known_topic_config_properties == ['some_config_key']


def test_get_pillar_topics_for_vtype_compute_when_update_is_allowed():
    check_get_pillar_topics_when_update_is_allowed('compute', mdb_topic_service_pb2)


def test_get_pillar_topics_for_aws_compute_when_update_is_allowed():
    check_get_pillar_topics_when_update_is_allowed('aws', datacloud_topic_service_pb2)
