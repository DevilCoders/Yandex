# -*- coding: utf-8 -*-

import json
import logging
import logging.handlers
import socket
import sys
import time
import tenacity
from typing import Dict
from confluent_kafka import KafkaException
from confluent_kafka.admin import AdminClient, ConfigResource, ConfigSource
from py4j.java_gateway import JavaGateway, get_field
from py4j.protocol import Py4JJavaError

from yandex.cloud.priv.mdb.kafka.v1.inner import (
    topic_service_pb2 as mdb_topic_service_pb2,
    topic_service_pb2_grpc as mdb_topic_service_pb2_grpc,
)
from datacloud.kafka.inner.v1 import (
    topic_service_pb2 as datacloud_topic_service_pb2,
    topic_service_pb2_grpc as datacloud_topic_service_pb2_grpc,
)
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.iam import jwt
from cloud.mdb.internal.python.logs import MdbLoggerAdapter


class MyEncoder(json.JSONEncoder):
    def default(self, o):
        return o.__dict__


class Topic:
    name: str
    partitions: int
    replication_factor: int
    config: dict

    def __init__(self, name: str, partitions: int, replication_factor: int, config: dict):
        self.name = name
        self.partitions = partitions
        self.replication_factor = replication_factor
        self.config = config

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def to_json(self):
        return json.dumps(self, default=lambda o: o.__dict__, sort_keys=True, indent=4)


def is_zero(val):
    return val == type(val)()


def config_value_from_string(val):
    if val == "true":
        return True
    if val == "false":
        return False
    try:
        return int(val)
    except ValueError:
        return val


def split_into_chunks(lst, n):
    """Yield successive n-sized chunks from lst."""
    for i in range(0, len(lst), n):
        yield lst[i : i + n]


def passed_since(moment):
    return round(time.time() - moment, 2)


class UpdateIsNotAllowed(RuntimeError):
    """
    Update request is not allowed
    """


class GetMetricError(RuntimeError):
    """
    Update request is not allowed
    """


class TopicSync:
    def __init__(self, config: dict) -> None:
        self.cluster_id = config['cluster_id']
        topic_sync_config = config.get('topic_sync', {})
        self.reload_kafka_topics_interval = topic_sync_config.get('reload_kafka_topics_interval', 5)
        self.logger = self.build_logger(config['logging'])
        self.is_aws = config.get('vtype') == 'aws'

        if self.is_aws:
            self.topic_service_pb2 = datacloud_topic_service_pb2
            self.topic_service_pb2_grpc = datacloud_topic_service_pb2_grpc
        else:
            self.topic_service_pb2 = mdb_topic_service_pb2
            self.topic_service_pb2_grpc = mdb_topic_service_pb2_grpc

        sa = config['service_account']
        sa_creds = jwt.SACreds(
            service_account_id=sa['id'],
            key_id=sa['key_id'],
            private_key=sa['private_key'],
        )

        jwt_config = config['iam_jwt']
        self.iam_jwt = jwt.IamJwt(
            config=jwt.Config(
                transport=grpcutil.Config(
                    server_name=jwt_config.get('server_name'),
                    url=jwt_config['url'],
                    cert_file=jwt_config.get('cert_file', '/opt/yandex/allCAs.pem'),
                    insecure=jwt_config.get('insecure', False),
                ),
                audience=jwt_config.get('audience', 'https://iam.api.cloud.yandex.net/iam/v1/tokens'),
                request_expire=jwt_config.get('request_expire', 3600),
                expire_thresh=jwt_config.get('expire_thresh', 180),
            ),
            logger=MdbLoggerAdapter(self.logger, extra={}),
            sa_creds=sa_creds,
        )

        intapi = config['intapi']
        channel = grpcutil.new_grpc_channel(
            grpcutil.Config(
                url=intapi['url'],
                cert_file=intapi.get('cert_file'),
                server_name=intapi.get('server_name'),
                insecure=intapi.get('insecure'),
            )
        )
        self.topic_service = grpcutil.WrappedGRPCService(
            MdbLoggerAdapter(self.logger, extra={}),
            channel,
            self.topic_service_pb2_grpc.TopicServiceStub,
            timeout=intapi.get('timeout', 10),
            get_token=self.iam_jwt.get_token,
            error_handlers={},
        )

        if 'kafka' in config:
            kafka = config['kafka']
            params = {
                'bootstrap.servers': socket.gethostname() + ':9091',
                'request.timeout.ms': kafka.get('timeout', 10 * 1000),
                'security.protocol': 'SASL_SSL',
                'ssl.ca.location': kafka.get('ca_path', '/etc/kafka/ssl/cert-ca.pem'),
                'sasl.mechanism': 'SCRAM-SHA-512',
                'sasl.username': kafka.get('username', 'mdb_admin'),
                'sasl.password': kafka['password'],
                'broker.address.family': kafka.get('address_family', 'any'),
            }
            self.kafka_admin_client = AdminClient(params)

        self.pillar_topics = {}
        self.revision = 0
        self.pillar_topics_updated_at = 0
        self.force_reload_pillar_topics_interval = topic_sync_config.get('force_reload_pillar_topics_interval', 3600)
        self.known_topic_config_properties = []
        self.local_broker_is_controller = None

        self.last_healthy_checkpoint_ts = int(time.time())
        self.last_topic_count = 0
        self.chunk_size = topic_sync_config.get('chuck_size', 100)
        self.report_if_unhealthy_for = topic_sync_config.get('report_if_unhealthy_for', 60)

    @staticmethod
    def build_logger(config):
        logger = logging.getLogger('topic_sync')
        logger.setLevel(config.get('level', 'INFO'))

        if sys.stdin.isatty():
            handler = logging.StreamHandler(sys.stdout)
        elif 'file' in config:
            handler = logging.handlers.RotatingFileHandler(config['file'], maxBytes=config.get('size', 104857600))
        else:
            handler = logging.NullHandler()
        formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(name)s:\t%(message)s')
        handler.setFormatter(formatter)
        logger.addHandler(handler)
        return logger

    def run(self):
        self.reload_pillar_topics()

        while True:
            self.wait_until_local_broker_is_controller()
            started_at = time.time()
            try:
                kafka_topics = self.get_kafka_topics()
            except KafkaException as e:
                self.logger.error(f"Failed to load Kafka topics: {e}")
                continue
            self.logger.info(f"Time to load Kafka topics info: {passed_since(started_at)}")

            started_at = time.time()
            changes = self.compare_all_topics(self.pillar_topics, kafka_topics)
            self.logger.debug(f"Time to compare Kafka and pillar topics: {passed_since(started_at)}")

            sleep = True
            if changes:
                req = self.build_update_request(changes, self.revision)
                self.logger.info(f'Sending following update to the server:\n{req}')
                update_accepted = self.send_update(req)
                if update_accepted:
                    self.logger.info("Update request was accepted, changes were saved to pillar.")
                else:
                    self.logger.info(
                        "Update request was rejected because topics cache is outdated" " or there's task in progress."
                    )
                    sleep = False
                self.reload_pillar_topics()
            else:
                self.logger.info("No differences were found between Kafka and pillar topics")
                if self.pillar_topics_outdated():
                    self.reload_pillar_topics()
            self.last_healthy_checkpoint_ts = int(time.time())
            if sleep:
                time.sleep(self.reload_kafka_topics_interval)

    def wait_until_local_broker_is_controller(self):
        sleep = 1.0
        max_interval = 30.0
        while True:
            try:
                # Metric ActiveControllerCount allows us to determine whether local broker is a controller:
                # * if there's no active controller in cluster then object
                #   kafka.controller:type=KafkaController,name=ActiveControllerCount is missing
                # * if there's a controller in cluster and it is local broker then ActiveControllerCount=1
                # * if there's a controller in cluster and it is some other broker then ActiveControllerCount=0
                active_controller_count = self.get_metric(
                    'kafka.controller:type=KafkaController,name=ActiveControllerCount'
                )
                if active_controller_count == '1':
                    broker_state = self.get_metric('kafka.server:type=KafkaServer,name=BrokerState')
                    # BrokerState=3 means RUNNING
                    # https://github.com/apache/kafka/blob/trunk/metadata/src/main/java/org/apache/kafka/metadata/BrokerState.java#L67
                    if broker_state == '3':
                        if not self.local_broker_is_controller:
                            self.logger.info("Local broker is a controller and is fully initialized.")
                            self.local_broker_is_controller = True
                        return
                    else:
                        self.logger.info(
                            f"Local broker is a controller but its state is {broker_state}."
                            f" Waiting for it to fully initialize ..."
                        )
                        self.local_broker_is_controller = None
                else:
                    if self.local_broker_is_controller in (True, None):
                        self.logger.info("Local broker is not a controller. Topic sync will be paused.")
                    self.local_broker_is_controller = False
            except GetMetricError as e:
                self.logger.warning(f"Failed to get metric: {e}")
                self.local_broker_is_controller = None
            except Py4JJavaError as e:
                self.logger.error(f"Call to JavaGateway raised an exception: {e}")
                self.local_broker_is_controller = None

            time.sleep(sleep)
            sleep = min(sleep * 2, max_interval)

    @staticmethod
    def get_metric(metric_name):
        gateway = JavaGateway()
        resp = gateway.entry_point.get_metric(metric_name)
        if get_field(resp, 'errorCode'):
            raise GetMetricError(get_field(resp, 'errorMessage'))
        else:
            return get_field(resp, 'value')

    @tenacity.retry(wait=tenacity.wait_exponential(multiplier=1, min=1, max=30))
    def reload_pillar_topics(self):
        pillar_topics, revision, update_allowed = self.get_pillar_topics()
        self.last_healthy_checkpoint_ts = int(time.time())
        if not update_allowed:
            self.logger.info("Update is not allowed. Repeating List request until update will be allowed.")
            raise UpdateIsNotAllowed
        self.pillar_topics = pillar_topics
        self.revision = revision
        self.pillar_topics_updated_at = time.time()

    # After support for a new topic config property is implemented within Cloud API we have to `reload_pillar_topics`
    # in order to save the value for this property.
    def pillar_topics_outdated(self):
        return time.time() - self.pillar_topics_updated_at > self.force_reload_pillar_topics_interval

    @tenacity.retry(wait=tenacity.wait_exponential(multiplier=1, min=1, max=30))
    def get_pillar_topics(self) -> (Dict[str, Topic], int, bool):
        try:
            req = self.topic_service_pb2.ListTopicsRequest()
            req.cluster_id = self.cluster_id
            resp = self.topic_service.List(req)
            if not resp.update_allowed:
                return {}, 0, False
            pillar_topics = {}
            for topic_json in resp.topics:
                topic_dict = json.loads(topic_json)
                topic = Topic(
                    name=topic_dict['name'],
                    partitions=topic_dict['partitions'],
                    replication_factor=topic_dict['replication_factor'],
                    config=topic_dict.get('config', {}),
                )
                pillar_topics[topic.name] = topic
            self.known_topic_config_properties = resp.known_topic_config_properties
            return pillar_topics, resp.revision, True
        except Exception as e:
            self.logger.error(f"Failed to get a list of topics from intapi: {e}")
            raise

    def get_kafka_topics(self) -> Dict[str, Topic]:
        topics = {}
        started_at = time.time()
        kafka_topics = self.kafka_admin_client.list_topics(timeout=10).topics
        self.last_topic_count = len(kafka_topics)
        self.logger.debug(f"Time to load a list of {self.last_topic_count} Kafka topics: {passed_since(started_at)}")

        # If cluster has many topics then time to load their configs becomes significant and it may affect server load.
        # To avoid this we are going to load configs in batches and to take pauses between loading consecutive batches.
        chunks = list(split_into_chunks(list(kafka_topics.keys()), self.chunk_size))
        for chunk in chunks:
            started_at = time.time()
            configs = self.get_kafka_topic_configs(chunk)
            for name in chunk:
                kafka_topic = kafka_topics[name]
                topic = Topic(
                    name=name,
                    partitions=len(kafka_topic.partitions),
                    replication_factor=min([len(partition.replicas) for partition in kafka_topic.partitions.values()]),
                    config={},
                )
                cfg = configs[name]
                for config_entry in cfg.values():
                    if config_entry.source == ConfigSource.DYNAMIC_TOPIC_CONFIG.value:
                        topic.config[config_entry.name] = config_value_from_string(config_entry.value)
                topics[name] = topic

            spent = passed_since(started_at)
            self.logger.debug(f"Time to load configs for the next batch of topics: {spent}")
            if len(chunks) > 1:
                delay = 10 * spent
                self.logger.debug(f"Going to sleep for {delay}")
                time.sleep(delay)

        return topics

    def get_kafka_topic_configs(self, topic_names):
        resources = []
        resource_by_topic_name = {}
        for name in topic_names:
            resource = ConfigResource(ConfigResource.Type.TOPIC, name)
            resources.append(resource)
            resource_by_topic_name[name] = resource
        response = self.kafka_admin_client.describe_configs(resources, request_timeout=10)
        configs = {}
        for name in topic_names:
            resource = resource_by_topic_name[name]
            config = response[resource].result()
            configs[name] = config
        return configs

    def compare_all_topics(self, pillar_topics: Dict[str, Topic], kafka_topics: Dict[str, Topic]):
        kafka_topic_names = set(kafka_topics.keys())
        pillar_topic_names = set(pillar_topics.keys())
        new_topic_names = kafka_topic_names - pillar_topic_names
        deleted_topic_names = pillar_topic_names - kafka_topic_names
        potentially_changed_topic_names = pillar_topic_names.intersection(kafka_topic_names)

        changes = {}
        skipped_topic_names = []

        if new_topic_names:
            new_topics = []
            for name in new_topic_names:
                if name.startswith('_'):
                    skipped_topic_names.append(name)
                    continue
                self.logger.info(f'Following topic will be added to pillar: {kafka_topics[name].__dict__}')
                new_topics.append(kafka_topics[name])
            if new_topics:
                changes['new'] = new_topics

        if skipped_topic_names and self.logger.isEnabledFor(logging.DEBUG):
            self.logger.debug(f'Service topics that will not be saved to pillar: {", ".join(skipped_topic_names)}')

        if deleted_topic_names:
            self.logger.info(f'Following topics will be deleted from pillar: {", ".join(deleted_topic_names)}')
            changes['deleted'] = deleted_topic_names

        changed_topics = []
        for topic_name in potentially_changed_topic_names:
            kafka_topic = kafka_topics[topic_name]
            pillar_topic = pillar_topics[topic_name]
            topic_changes, kafka_topic = self.compare_kafka_and_pillar_topic(kafka_topic, pillar_topic)
            if topic_changes:
                self.logger.info(f'Following changes will be made to topic {topic_name} within pillar: {topic_changes}')
                changed_topics.append(kafka_topic)
        if changed_topics:
            changes['updated'] = changed_topics

        return changes

    def compare_kafka_and_pillar_topic(self, kafka_topic: Topic, pillar_topic: Topic) -> (str, Topic):
        if 'min.insync.replicas' in kafka_topic.config:
            min_insync_replicas = kafka_topic.config['min.insync.replicas']
            expected = 1 if kafka_topic.replication_factor < 3 else 2
            if 'min.insync.replicas' in pillar_topic.config:
                # min.insync.replicas is specified for topic explicitly: we will take this param into account
                # when comparing configs
                pass
            elif min_insync_replicas == expected:
                # min.insync.replicas is set to the value that we would specify in salt module: do not take
                # it into account when comparing configs
                del kafka_topic.config['min.insync.replicas']
            else:
                # min.insync.replicas is set to the value that differs from one that we would specify in salt module:
                # update it on server
                pass

        changes = []
        if kafka_topic.partitions != pillar_topic.partitions:
            changes.append(f'partitions will be changed from {pillar_topic.partitions} to {kafka_topic.partitions}')
        if kafka_topic.replication_factor != pillar_topic.replication_factor:
            changes.append(
                f'replication_factor will be changed from {pillar_topic.replication_factor} to {kafka_topic.replication_factor}'
            )
        kafka_props = set(kafka_topic.config.keys())
        pillar_props = set(pillar_topic.config.keys())
        deleted_props = pillar_props - kafka_props
        for prop in deleted_props:
            changes.append(f'{prop} will be deleted')
        for name in list(kafka_topic.config):
            new_value = kafka_topic.config[name]
            if name in pillar_topic.config:
                old_value = pillar_topic.config[name]
                if new_value != old_value:
                    changes.append(f'{name} will be changed from {old_value} to {new_value}')
            elif name not in self.known_topic_config_properties:
                # This property is not known to Cloud API. For such properties we do not indicate change
                # and do not send update request to Cloud API. Otherwise kafka-agent would infinitely
                # try to save such an update.
                del kafka_topic.config[name]
            else:
                # Currently we do not serialize to json config properties that have a zero value of corresponding
                # type. Such properties will be missing from pillar_topic.config, but may be present in
                # kafka_topic.config. Ignore them in order to avoid infinite update attempt.
                if is_zero(new_value):
                    del kafka_topic.config[name]
                else:
                    changes.append(f'{name} will be set to {new_value}')

        return ', '.join(changes), kafka_topic

    def build_update_request(self, changes, revision):
        req = self.topic_service_pb2.UpdateTopicsRequest()
        req.cluster_id = self.cluster_id
        req.revision = revision
        changed_topics = changes.get('new', []) + changes.get('updated', [])
        req.changed_topics.extend([topic.to_json() for topic in changed_topics])
        req.deleted_topics.extend(changes.get('deleted', []))
        return req

    @tenacity.retry(wait=tenacity.wait_exponential(multiplier=1, min=1, max=30))
    def send_update(self, req):
        try:
            resp = self.topic_service.Update(req)
        except Exception as e:
            self.logger.error(f"Failed to send update to intapi: {e}")
            raise
        return resp.update_accepted

    def healthy_period(self):
        return (self.last_topic_count // self.chunk_size + 1) * self.report_if_unhealthy_for

    def status(self):
        ok = True
        if time.time() - self.last_healthy_checkpoint_ts > self.healthy_period():
            ok = False
        if not ok and self.local_broker_is_controller is False:
            ok = True

        return {
            'ok': ok,
            'last_healthy_checkpoint_ts': self.last_healthy_checkpoint_ts,
            'local_broker_is_controller': self.local_broker_is_controller,
        }
