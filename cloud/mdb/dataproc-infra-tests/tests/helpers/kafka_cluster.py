"""
Utilities for dealing with Kafka cluster
"""

import base64
import json
import logging
import time

import grpc
from google.protobuf import json_format

from confluent_kafka import Consumer, Producer
from confluent_kafka.admin import AdminClient, ConfigResource, NewPartitions, NewTopic

from tests.helpers.step_helpers import (
    fill_update_mask,
    get_step_data,
    apply_overrides_to_cluster_config,
)
from tests.helpers.utils import ssh
from yandex.cloud.priv.mdb.kafka.v1 import (
    cluster_service_pb2,
    cluster_service_pb2_grpc,
    connector_service_pb2,
    connector_service_pb2_grpc,
    topic_service_pb2,
    topic_service_pb2_grpc,
    user_service_pb2,
    user_service_pb2_grpc,
)

from tests.helpers.base_cluster import BaseCluster

LOG = logging.getLogger('kafka_cluster')


class KafkaCluster(BaseCluster):
    def __init__(self, context):
        super().__init__(context)
        self.infratest_hostname = context.conf['compute_driver']['fqdn']
        self.username = 'root'
        host = context.conf['projects']['mdb_internal_api']['host']
        port = context.conf['projects']['mdb_internal_api']['port']
        self.url = f'{host}:{port}'
        self.channel = grpc.insecure_channel(self.url)
        self.cluster_service = cluster_service_pb2_grpc.ClusterServiceStub(self.channel)
        self.topic_service = topic_service_pb2_grpc.TopicServiceStub(self.channel)
        self.user_service = user_service_pb2_grpc.UserServiceStub(self.channel)
        self.connector_service = connector_service_pb2_grpc.ConnectorServiceStub(self.channel)

    def connect_string(self, hosts):
        brokers = []
        for host in hosts:
            if host['role'] != 'KAFKA':
                continue
            brokers.append(host['name'] + ':9091')
        return ','.join(brokers)

    def load_cluster_into_context(self, cluster_id=None, name=None):
        if not cluster_id:
            if not name:
                raise ValueError('Load into context must specify name or cid')
            clusters = self.list_clusters()
            for cluster in clusters:
                if cluster['name'] == name:
                    cluster_id = cluster['id']
        self.context.cid = cluster_id
        if not cluster_id:
            raise ValueError('Cluster not found by name: ', name)
        self.context.cluster = self.get_cluster(cluster_id)
        if not hasattr(self.context, "cluster_connect_strings"):
            self.context.cluster_connect_strings = {}
        hosts = self.list_hosts(self.context.cluster['id'], rewrite_context_with_response=False)
        self.context.cluster_connect_strings[self.context.cluster['name']] = self.connect_string(hosts)

    def get_cluster(self, cluster_id, rewrite_context_response=False):
        request = cluster_service_pb2.GetClusterRequest(cluster_id=cluster_id)
        response = self._make_request(self.cluster_service.Get, request, rewrite_context_response)
        return json_format.MessageToDict(response)

    def list_clusters(self, folder_id: str = None) -> list:
        if not folder_id:
            folder_id = self.context.folder['folder_ext_id']
        request = cluster_service_pb2.ListClustersRequest(folder_id=folder_id)
        response = self._make_request(self.cluster_service.List, request, rewrite_context_with_response=False)
        return json_format.MessageToDict(response).get('clusters', [])

    def create_cluster(self, cluster_name):
        cluster_config = apply_overrides_to_cluster_config(self.context.cluster_config, get_step_data(self.context))
        cluster_config['name'] = cluster_name
        cluster_config['folderId'] = self.context.folder['folder_ext_id']

        request = cluster_service_pb2.CreateClusterRequest()
        json_format.ParseDict(cluster_config, request)
        response = self._make_request(self.cluster_service.Create, request)
        json_data = json_format.MessageToDict(response)
        self.context.operation_id = response.id
        self.context.cid = json_data['metadata']['clusterId']
        self.context.cluster = self.get_cluster(self.context.cid, rewrite_context_response=False)

        for _ in range(5):
            try:
                self.load_cluster_into_context(cluster_id=self.context.cid)
                return
            except Exception:
                pass
            time.sleep(1)

        return response

    def stop_cluster(self, cluster_name):
        request = cluster_service_pb2.StopClusterRequest(cluster_id=self.context.cid)
        return self._make_request(self.cluster_service.Stop, request)

    def start_cluster(self, cluster_name):
        request = cluster_service_pb2.StartClusterRequest(cluster_id=self.context.cid)
        return self._make_request(self.cluster_service.Start, request)

    def update_cluster(self):
        cluster_config = get_step_data(self.context)
        if 'cluster_id' not in cluster_config:
            cluster_config['cluster_id'] = self.context.cid

        request = cluster_service_pb2.UpdateClusterRequest()
        fill_update_mask(request, cluster_config)
        json_format.ParseDict(cluster_config, request)
        return self._make_request(self.cluster_service.Update, request)

    def delete_cluster(self):
        request = cluster_service_pb2.DeleteClusterRequest(cluster_id=self.context.cid)
        return self._make_request(self.cluster_service.Delete, request)

    def list_hosts(self, cluster_id: str = None, rewrite_context_with_response=True) -> list:
        if not cluster_id:
            cluster_id = self.context.cid
        request = cluster_service_pb2.ListClusterHostsRequest(cluster_id=cluster_id)
        # TODO pagination support
        response = self._make_request(
            self.cluster_service.ListHosts, request, rewrite_context_with_response=rewrite_context_with_response
        )
        return json_format.MessageToDict(response).get('hosts', [])

    def get_topic(self, name):
        request = topic_service_pb2.GetTopicRequest(cluster_id=self.context.cid, topic_name=name)
        response = self._make_request(self.topic_service.Get, request)
        return json_format.MessageToDict(response, preserving_proto_field_name=True)

    def create_topic(self):
        topic_config = get_step_data(self.context)
        if 'cluster_id' not in topic_config:
            topic_config['cluster_id'] = self.context.cid

        request = topic_service_pb2.CreateTopicRequest()
        json_format.ParseDict(topic_config, request)
        return self._make_request(self.topic_service.Create, request)

    def update_topic(self):
        topic_config = get_step_data(self.context)
        if 'cluster_id' not in topic_config:
            topic_config['cluster_id'] = self.context.cid

        request = topic_service_pb2.UpdateTopicRequest()
        json_format.ParseDict(topic_config, request)
        return self._make_request(self.topic_service.Update, request)

    def delete_topic(self, topic_name: str, cluster_id: str = None):
        if not cluster_id:
            cluster_id = self.context.cid
        request = topic_service_pb2.DeleteTopicRequest(topic_name=topic_name, cluster_id=cluster_id)
        return self._make_request(self.topic_service.Delete, request)

    def create_user(self):
        user_config = get_step_data(self.context)
        if 'cluster_id' not in user_config:
            user_config['cluster_id'] = self.context.cid

        request = user_service_pb2.CreateUserRequest()
        json_format.ParseDict(user_config, request)
        return self._make_request(self.user_service.Create, request)

    def update_user(self):
        user_config = get_step_data(self.context)
        if 'cluster_id' not in user_config:
            user_config['cluster_id'] = self.context.cid

        request = user_service_pb2.UpdateUserRequest()
        json_format.ParseDict(user_config, request)
        return self._make_request(self.user_service.Update, request)

    def delete_user(self, user_name: str, cluster_id: str = None):
        if not cluster_id:
            cluster_id = self.context.cid

        request = user_service_pb2.DeleteUserRequest(cluster_id=cluster_id, user_name=user_name)
        return self._make_request(self.user_service.Delete, request)

    def common_options(self, user, errors):
        def error_cb(err):
            nonlocal errors
            errors.append(err)

        return {
            'bootstrap.servers': ','.join([f'{hostname}:9091' for hostname in self.get_broker_hostnames()]),
            'security.protocol': 'SASL_SSL',
            'sasl.mechanism': 'SCRAM-SHA-512',
            'sasl.password': f'{user}-password',
            'sasl.username': user,
            'ssl.ca.location': 'staging/infratest.pem',
            'error_cb': error_cb,
        }

    def produce_message(self, user: str, topic: str, message: str):
        delivered = False
        errors = []
        producer = Producer(self.common_options(user, errors))

        def delivery_callback(err, msg):
            if err:
                nonlocal errors
                errors.append(err)
                return
            nonlocal delivered
            delivered = True

        producer.produce(topic, message, on_delivery=delivery_callback)
        producer.flush(3)
        producer.poll(1)

        return delivered, errors

    def consume_message(self, user, topic, timeout=3):
        errors = []
        options = self.common_options(user, errors)
        options.update(
            {
                'auto.offset.reset': 'beginning',
                'group.id': 'group1',
            }
        )
        consumer = Consumer(options)
        consumer.subscribe([topic])
        msg = consumer.poll(timeout)
        if msg is None:
            return None, errors
        elif msg.error():
            return None, [msg.error()]
        else:
            val = msg.value()
            try:
                obj = json.loads(val)
                if obj['schema']['type'] == 'bytes':
                    val = base64.b64decode(obj['payload'])
            except json.decoder.JSONDecodeError:
                pass
            except KeyError:
                pass
            return val, errors

    def create_topic_via_admin_api(self, user, topic_name, partitions, replication_factor, config):
        errors = []
        client = AdminClient(self.common_options(user, errors))
        res = client.create_topics([NewTopic(topic_name, partitions, replication_factor, config=config)])
        future = res[topic_name]
        return future.result()

    def update_topic_via_admin_api(self, user, topic_name, partitions, config):
        errors = []
        client = AdminClient(self.common_options(user, errors))

        if partitions:
            res = client.create_partitions([NewPartitions(topic_name, partitions)])
            res[topic_name].result()

        resource = ConfigResource(ConfigResource.Type.TOPIC, topic_name, set_config=config)
        result = client.alter_configs([resource])
        return result[resource].result()

    def delete_topic_via_admin_api(self, user, topic_name):
        errors = []
        client = AdminClient(self.common_options(user, errors))
        res = client.delete_topics([topic_name])
        future = res[topic_name]
        return future.result()

    def get_broker_config(self, user, broker_id):
        errors = []
        client = AdminClient(self.common_options(user, errors))
        resource = ConfigResource(ConfigResource.Type.BROKER, str(broker_id))
        res = client.describe_configs([resource])
        return res[resource].result()

    def get_broker_hostnames(self) -> list:
        return [host['name'] for host in self.list_hosts() if host['role'] == 'KAFKA']

    def on_gateway(self, cmd: str):
        self.context.code, self.context.out, self.context.err = ssh(self.infratest_hostname, [cmd], options=('-A',))
        return self.context.code, self.context.out, self.context.err

    def create_connector(self):
        connector_config = get_step_data(self.context)
        if 'cluster_id' not in connector_config:
            connector_config['cluster_id'] = self.context.cid

        request = connector_service_pb2.CreateConnectorRequest()
        json_format.ParseDict(connector_config, request)
        return self._make_request(self.connector_service.Create, request)

    def delete_connector(self, connector_name: str, cluster_id: str = None):
        if not cluster_id:
            cluster_id = self.context.cid

        request = connector_service_pb2.DeleteConnectorRequest(cluster_id=cluster_id, connector_name=connector_name)
        return self._make_request(self.connector_service.Delete, request)

    def pause_connector(self, connector_name: str, cluster_id: str = None):
        if not cluster_id:
            cluster_id = self.context.cid

        request = connector_service_pb2.PauseConnectorRequest(cluster_id=cluster_id, connector_name=connector_name)
        return self._make_request(self.connector_service.Pause, request)

    def resume_connector(self, connector_name: str, cluster_id: str = None):
        if not cluster_id:
            cluster_id = self.context.cid

        request = connector_service_pb2.ResumeConnectorRequest(cluster_id=cluster_id, connector_name=connector_name)
        return self._make_request(self.connector_service.Resume, request)

    def update_connector(self):
        connector_config = get_step_data(self.context)
        if 'cluster_id' not in connector_config:
            connector_config['cluster_id'] = self.context.cid
        request = connector_service_pb2.UpdateConnectorRequest()
        json_format.ParseDict(connector_config, request)
        return self._make_request(self.connector_service.Update, request)
