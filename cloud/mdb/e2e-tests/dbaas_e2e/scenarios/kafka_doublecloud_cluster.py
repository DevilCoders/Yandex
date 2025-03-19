"""
DBaaS E2E tests scenarios for Kafka in datacloud
"""

import json
import logging
import time

from confluent_kafka import Consumer, Producer
from confluent_kafka.admin import AdminClient, NewTopic

from datacloud.kafka.v1 import cluster_service_pb2 as cs_spec
from datacloud.kafka.v1.cluster_service_pb2_grpc import ClusterServiceStub
from datacloud.kafka.v1 import (
    topic_service_pb2,
    topic_service_pb2_grpc,
)

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.grpcutil import exceptions
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from ..utils import scenario


class KafkaDoubleCloudBaseClusterCreate:
    """
    Scenario for Kafka creation in double cloud
    """

    CLUSTER_TYPE = 'kafka'
    CLUSTER_NAME_SUFFIX = '_double_cloud'
    API_CLIENT = 'doublecloud-internal-kafka-grpc'

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

    @classmethod
    def get_options(cls, config):
        """
        Returns options for cluster creation
        """
        resource_preset_id = config.resource_preset_id if cls.ARCHITECTURE == 'amd' else config.arm_resource_preset_id
        options = {
            'project_id': config.project_id,
            'cloud_type': config.cloud_type,
            'region_id': config.region_id,
            'name': config.name,
            'description': config.description,
            'version': config.version,
            'resources': {
                'kafka': {
                    'resource_preset_id': resource_preset_id,
                    'disk_size': config.disk_size,
                    'broker_count': 1,
                    'zone_count': 1,
                },
            },
            'access': {
                'ipv4_cidr_blocks': {
                    'values': [
                        {
                            'value': '0.0.0.0/0',
                            'description': 'access for e2e-tests',
                        }
                    ],
                },
                'ipv6_cidr_blocks': {
                    'values': [
                        {
                            'value': '::/0',
                            'description': 'access for e2e-tests',
                        }
                    ],
                },
            },
            'encryption': {
                'enabled': config.encryption,
            },
            'network_id': config.network_id,
        }

        return options

    @classmethod
    def error_callback(cls, err):
        cls.logger.error(f'Error: {err}')

    @classmethod
    def delivery_callback(cls, err, msg):
        if not err:
            return
        cls.logger.error(f'Message failed delivery: {err}')

    @classmethod
    def post_check(cls, config, hosts, logger, cluster_id, api_client, **_):
        connection_string, user, password = cls.get_connection_info(logger, cluster_id, api_client)
        connect_params = cls.kafka_connect_params(connection_string, user, password, config.conn_ca_path, logger)

        cls.check_topic_sync(connect_params, logger, cluster_id, api_client)
        cls.check_kafka_produce_and_consume(connect_params, logger)

    @classmethod
    def kafka_connect_params(cls, connection_string, user, password, ca_path, logger, extra=None):
        params = {
            'bootstrap.servers': connection_string,
            'security.protocol': 'SASL_SSL',
            'sasl.mechanism': 'SCRAM-SHA-512',
            'sasl.password': password,
            'sasl.username': user,
            'ssl.ca.location': ca_path,
            'broker.address.family': 'v4',
            'error_cb': cls.error_callback,
            'logger': logger,
        }
        params.update(extra or {})
        return params

    @classmethod
    def get_connection_info(cls, logger, cluster_id, api_client):
        cluster_service = grpcutil.WrappedGRPCService(
            MdbLoggerAdapter(logger, extra={}),
            api_client._grpc_channel,
            ClusterServiceStub,
            timeout=api_client._request_timeout,
            get_token=api_client._token,
            error_handlers={},
        )

        req = cs_spec.GetClusterRequest(cluster_id=cluster_id, sensitive=True)
        resp = cluster_service.Get(req)
        connection = resp.connection_info
        return connection.connection_string, connection.user, connection.password

    @classmethod
    def check_topic_sync(cls, connect_params, logger, cluster_id, api_client, **_):
        """
        Check that topic modifications made via Kafka Admin API are synced to Cloud API
        """
        cls.logger = logger

        admin_client = AdminClient(connect_params)
        topic_name = "events"
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
    def check_kafka_produce_and_consume(cls, connect_params, logger, **_):
        """
        Produce some messages to Kafka and them consume them back
        """
        cls.logger = logger
        producer = Producer(connect_params)

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

        consumer_params = {'group.id': 'group1', 'auto.offset.reset': 'earliest'}
        consumer_params.update(connect_params)
        consumer = Consumer(consumer_params)
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


@scenario
class KafkaDoubleCloudAMDClusterCreate(KafkaDoubleCloudBaseClusterCreate):

    CLUSTER_NAME_SUFFIX = '_double_cloud_amd'
    ARCHITECTURE = 'amd'


@scenario
class KafkaDoubleCloudARMClusterCreate(KafkaDoubleCloudBaseClusterCreate):

    CLUSTER_NAME_SUFFIX = '_double_cloud_arm'
    ARCHITECTURE = 'arm'
