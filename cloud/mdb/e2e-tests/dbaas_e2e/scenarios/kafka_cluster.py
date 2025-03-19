"""
DBaaS E2E tests scenarios for Kafka
"""

import json
import logging
import time

from confluent_kafka import Consumer, Producer
from confluent_kafka.admin import AdminClient, NewTopic

from yandex.cloud.priv.mdb.kafka.v1 import cluster_service_pb2 as cs_spec
from yandex.cloud.priv.mdb.kafka.v1 import user_pb2
from yandex.cloud.priv.mdb.kafka.v1.cluster_service_pb2_grpc import ClusterServiceStub
from yandex.cloud.priv.mdb.kafka.v1 import (
    topic_service_pb2,
    topic_service_pb2_grpc,
)

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.grpcutil import exceptions
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from ..utils import geo_name, scenario


@scenario
class KafkaClusterCreate:
    """
    Scenario for Kafka creation
    """

    CLUSTER_TYPE = 'kafka'
    API_CLIENT = 'internal-grpc'

    logger = logging

    @staticmethod
    def get_api_client_kwargs():
        return {
            'grpc_defs': {
                'cluster_service': {
                    'stub': ClusterServiceStub,
                    'requests': {
                        'create': cs_spec.CreateClusterRequest,
                        'delete': cs_spec.DeleteClusterRequest,
                        'list_clusters': cs_spec.ListClustersRequest,
                        'list_hosts': cs_spec.ListClusterHostsRequest,
                    },
                },
            },
        }

    @staticmethod
    def get_options(config):
        """
        Returns options for cluster creation
        """
        options = {
            'environment': config.environment,
            'folder_id': config.folder_id,
            'network_id': config.network_id,
            'name': 'kafka_e2e',
            'config_spec': {
                'kafka': {
                    'resources': {
                        'resource_preset_id': config.flavor,
                        'disk_size': 10737418240,
                        'disk_type_id': config.disk_type,
                    },
                },
                'zone_id': [geo_name(config, 'myt')],
                'brokers_count': 1,
                'assign_public_ip': False,
            },
            'topic_specs': [
                {
                    'name': 'events',
                    'partitions': 2,
                    'replication_factor': 1,
                },
                {
                    'name': 'sensors',
                    'partitions': '2',
                    'replication_factor': 1,
                },
            ],
            'user_specs': [
                {
                    'name': 'producer',
                    'password': config.dbpassword,
                    'permissions': [
                        {'role': user_pb2.Permission.AccessRole.ACCESS_ROLE_PRODUCER, 'topic_name': 'events'}
                    ],
                },
                {
                    'name': 'consumer',
                    'password': config.dbpassword,
                    'permissions': [
                        {'role': user_pb2.Permission.AccessRole.ACCESS_ROLE_CONSUMER, 'topic_name': 'events'}
                    ],
                },
            ],
        }

        if 'porto' not in config.dbname:
            options['user_specs'].append(
                {
                    'name': 'admin',
                    'password': config.dbpassword,
                    'permissions': [{'role': user_pb2.Permission.AccessRole.ACCESS_ROLE_ADMIN, 'topic_name': '*'}],
                }
            )

        return options

    @classmethod
    def error_callback(cls, err):
        """Log errors"""
        cls.logger.error(f'Error: {err}')

    @classmethod
    def delivery_callback(cls, err, msg):
        """Log errors"""
        if not err:
            return
        cls.logger.error(f'Message failed delivery: {err}')

    @classmethod
    def post_check(cls, config, hosts, logger, cluster_id, api_client, **_):
        """
        Post-creation check
        """
        cls.check_kafka_produce_and_consume(config, hosts, logger)
        cls.check_topic_sync(config, hosts, logger, cluster_id, api_client)

    @classmethod
    def kafka_connect_params(cls, config, hosts, user, logger, extra=None):
        all_hosts = [f"{x['name']}:9091" for x in hosts['hosts']]
        bootstrap_servers = ','.join(all_hosts)
        params = {
            'bootstrap.servers': bootstrap_servers,
            'security.protocol': 'SASL_SSL',
            'sasl.mechanism': 'SCRAM-SHA-512',
            'sasl.password': config.dbpassword,
            'sasl.username': user,
            'ssl.ca.location': config.conn_ca_path,
            'error_cb': cls.error_callback,
            'logger': logger,
        }
        params.update(extra or {})
        return params

    @classmethod
    def check_topic_sync(cls, config, hosts, logger, cluster_id, api_client, **_):
        """
        Check that topic modifications made via Kafka Admin API are synced to Cloud API
        """
        if 'porto' in config.dbname:
            return

        cls.logger = logger
        admin_client = AdminClient(cls.kafka_connect_params(config, hosts, 'admin', logger))
        topic_name = "admin_api_topic"
        res = admin_client.create_topics([NewTopic(topic_name, num_partitions=6, replication_factor=1, config={})])
        future = res[topic_name]
        future.result()

        topic_service = grpcutil.WrappedGRPCService(
            MdbLoggerAdapter(logger, extra={}),
            api_client._grpc_channel,
            topic_service_pb2_grpc.TopicServiceStub,
            timeout=api_client._request_timeout,
            get_token=api_client._token,
            error_handlers={},
        )

        deadline = time.time() + 60
        while True:
            try:
                req = topic_service_pb2.GetTopicRequest(cluster_id=cluster_id, topic_name=topic_name)
                resp = topic_service.Get(req)
            except exceptions.NotFoundError:
                resp = None
            if resp:
                break
            time.sleep(1)
            assert (
                time.time() < deadline
            ), f"Topic {topic_name} created via Admin API haven't been synced to Cloud API within timeout"

        assert resp.partitions.value == 6, f'Topic {topic_name} has {resp.partitions.value} partitions, expected 6'

    @classmethod
    def check_kafka_produce_and_consume(cls, config, hosts, logger, **_):
        """
        Produce some messages to Kafka and them consume them back
        """
        cls.logger = logger
        producer = Producer(cls.kafka_connect_params(config, hosts, 'producer', logger))

        event_ids = [7, 23, 37, 53, 61]
        for idx in event_ids:
            event = {'id': idx}
            value = json.dumps(event).encode('utf-8')
            producer.produce('events', value, on_delivery=cls.delivery_callback)
        messages_in_queue = producer.flush(10)  # wait no more than 10 seconds
        assert (
            messages_in_queue == 0
        ), f'Expected all messages to be delivered, but got {messages_in_queue} messages in queue'
        logger.info('All messages have been delivered')

        params = cls.kafka_connect_params(
            config, hosts, 'consumer', logger, {'group.id': 'group1', 'auto.offset.reset': 'earliest'}
        )
        consumer = Consumer(params)
        consumer.subscribe(['events'])

        got_ids = []
        timeout = 30
        while True:
            msg = consumer.poll(timeout)
            if msg is None:
                logger.info('No more messages to consume')
                break
            elif msg.error():
                logger.error(f'Error while consuming message: {msg.error()}')
            else:
                event = json.loads(msg.value())
                got_ids.append(event.get('id'))
                if len(got_ids) == len(event_ids):
                    # we expect that there are no more messages,
                    # so decrease poll timeout in order not to wait for too long
                    timeout = 5.0
        consumer.close()
        got_ids.sort()

        assert event_ids == got_ids, f'Expected to consume messages with ids {event_ids}, but got {got_ids}'
        logger.info('All messages sent by producer were successfully received by consumer')
