# -*- coding: utf-8 -*-
# All functions from this module
# are not for single usage.
# They should be used only
# in appropriate state
# with additional logic.
"""
Apache Kafka Connect utility functions for MDB
"""

import re
import logging

try:
    from urllib import quote  # Python 2.X
except ImportError:
    from urllib.parse import quote  # Python 3+

import requests
import socket

from contextlib import closing

try:
    from salt.exceptions import CommandExecutionError, TimeoutError
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

log = logging.getLogger(__name__)
CLUSTER_TYPE_THIS = 'this_cluster'
CERT_CA_FILE = '/etc/kafka/ssl/cert-ca.pem'
GET_METHOD = 'GET'
POST_METHOD = 'POST'
PUT_METHOD = 'PUT'
DELETE_METHOD = 'DELETE'
CLUSTER_TYPE_EXTERNAL = 'external'
YANDEX_CLOUD_ENV_SUFFIXES = {
    '.mdb.yandexcloud.net',  # PROD
    'db.yandex.net',  # PORTO-PROD
    'mdb.cloud-preprod.yandex.net',  # PREPROD
}
SASL_JAAS_CONFIG_CFG = 'sasl.jaas.config'
SECURITY_PROTOCOL_CFG = 'security.protocol'
SASL_MECHANISM_CFG = 'sasl.mechanism'
BOOTSTRAP_SERVERS_CFG = 'bootstrap.servers'
JSON_HEADERS = {'Content-Type': 'application/json', 'Accept': 'application/json'}
TASKS_MAX_CFG = 'tasks.max'
SSL_TRUSTSTORE_CERTIFICATES = 'ssl.truststore.certificates'


def __virtual__():
    return True


def sasl_admin_password():
    """
    :return: admin password for this Kafka cluster.
    """
    return __salt__['pillar.get']('data:kafka:admin_password')


def sasl_jaas_config_this_cluster():
    """
    :return: sasl.jaas.config for this Kafka cluster.
    """
    return jaas_config_make('mdb_admin', sasl_admin_password())


def yandex_cluster_common_security_creds():
    """
    :return: security creds that common for all MDB Kafka-clusters.
    """
    return {
        SECURITY_PROTOCOL_CFG: 'SASL_SSL',
        SASL_MECHANISM_CFG: 'SCRAM-SHA-512',
        'ssl.truststore.location': '/etc/kafka/ssl/server.truststore.jks',
        'ssl.truststore.password': sasl_admin_password(),
    }


def this_cluster_security_creds():
    """
    :return: security creds to connect to this Kafka cluster.
    """
    return {
        SASL_JAAS_CONFIG_CFG: sasl_jaas_config_this_cluster(),
        BOOTSTRAP_SERVERS_CFG: __salt__['mdb_kafka.kafka_connect_cluster_brokers_list'](),
    }


def make_request(endpoint, method, headers=None, json=None):
    """
    Make REST request with given params.
    :param data: data of REST-request.
    :param json: json data of REST-request.
    :param headers: headers of REST-request.
    :param endpoint: REST endpoint.
    :param method: name of REST method. For example 'GET'
    :return: response object.
    """
    fqdn = __salt__['grains.get']('id')
    url = 'https://' + fqdn + ':8083'
    url += endpoint
    with closing(requests.Session()) as session:
        session.verify = CERT_CA_FILE
        if headers:
            session.headers.update(headers)
        response = session.request(method=method, url=url, json=json)
        response.raise_for_status()
        return response


def escape_connector_name(name):
    """
    Escape name of connector to use it un url of REST-request.
    :param name: name of connector.
    :return: escaped name of connector, ready to insert into url of REST-request.
    """
    return quote(name)


def list_connectors():
    """
    Make REST GET-request for worker to receive list of all existing connectors at the moment.
    :return: list of all registered connectors at the moment.
    """
    response = make_request(endpoint='/connectors', method=GET_METHOD)
    connectors = re.findall("\"([\-\w\*]*)\"", response.text)
    return connectors


def connector_exists(name):
    """
    Check if connector with such name already exists.
    :param name: name of the connector.
    :return: True - if connector exists,
             False - otherwise.
    """
    return name in list_connectors()


def delete_connector(name):
    """
    Make rest DELETE-request for worker to delete connector with given name.
    :param name: connector's name.
    """
    endpoint = '/connectors/' + escape_connector_name(name)
    make_request(endpoint=endpoint, method=DELETE_METHOD)


def create_connector(name, config):
    """
    Create new connector with given name and config.
    Send POST-request with connector's data for worker.
    :param name: name of the connector.
    :param config: config of the connector.
    """
    rest_config = {'config': config, 'name': name}
    make_request(endpoint='/connectors', method=POST_METHOD, json=rest_config)


def get_connector_config(name):
    """
    Make REST GET-request for worker to receive config of connector.
    :param name: name of connector.
    :return: Dictionary - connector's config.
    """
    endpoint = '/connectors/' + escape_connector_name(name)
    response = make_request(endpoint=endpoint, method=GET_METHOD)
    return response.json()['config']


def connector_config_matches(name, config):
    """
    Check if existing connector with such name has that config.
    :param name: connector's name.
    :param config: config that we compare with current config of connector.
    :return: True - if config of existing connector matches with given config.
             False - otherwise.
    """
    return get_connector_config(name) == config


def update_connector_config(name, config):
    """
    Create REST PUT-request for Kafka connect-worker to update config of an existing connector.
    :param name: name of the connector.
    :param config: new config that we submit to connector.
    """
    endpoint = '/connectors/' + escape_connector_name(name) + '/config'
    make_request(endpoint=endpoint, method=PUT_METHOD, headers=JSON_HEADERS, json=config)


def parse_tasks_max(connector_data):
    """
    :return: correctly parsed value of 'tasks_max' connector property.
    """
    if TASKS_MAX_CFG in connector_data:
        tasks_max = connector_data[TASKS_MAX_CFG]
        return str(tasks_max)
    else:
        return '1'


def this_cluster_connection_config():
    """
    Kafka Connect worker is running on this cluster. This func return credentials
    for connecting to this cluster.
    Credentials:
    - bootstrap.servers
    - security.protocol
    - sasl.mechanism
    - ssl.truststore.location
    - ssl.truststore.password
    - sasl.jaas.config
    :return: Dictionary - correct set of credentials for connecting to this cluster.
    """
    result = {}
    result.update(yandex_cluster_common_security_creds())
    result.update(this_cluster_security_creds())
    return result


def trim_bootstrap_servers(bootstrap_servers):
    """
    Trim list of bootstrap servers, separated by ','. Remove all spaces in the begin, end of common string,
    and between bootstrap servers.
    For example: " bootstrapServer1, bootstrapServer2 " -> "bootstrapServer1,bootstrapServer2"
    :param bootstrap_servers: common string, contains all bootstrap server.
    :return: list of bootstrap_servers, separated by ",", without spaces.
    """
    bootstrap_servers = bootstrap_servers.strip()
    parts = bootstrap_servers.split(",")
    parts_temp = []
    for part in parts:
        parts_temp.append(part.strip())
    return ",".join(parts_temp)


def external_cluster_connection_config(data):
    """
    This func return credentials for connecting to external Kafka cluster.
    Credentials:
    - bootstrap.servers
    - security.protocol
    - sasl.mechanism (optional, for SASL_PLAINTEXT and SASL_SSL)
    - sasl.jaas.config (optional, for SASL_PLAINTEXT and SASL_SSL)
    - ssl.truststore.location (optional, for SASL_SSL)
    - ssl.truststore.password (optional, for SASL_SSL)
    :return: Dictionary - parsed set of credentials for connecting to external cluster.
    """
    bootstrap_servers = trim_bootstrap_servers(data[BOOTSTRAP_SERVERS_CFG])
    test_connection_to_external_cluster(bootstrap_servers)
    result = {BOOTSTRAP_SERVERS_CFG: bootstrap_servers}
    if is_yandex_cluster(bootstrap_servers):
        result.update(yandex_cluster_common_security_creds())
        result[SASL_JAAS_CONFIG_CFG] = jaas_config_make(data['sasl.username'], data['sasl.password'])
    else:
        security_protocol = data.get(SECURITY_PROTOCOL_CFG, 'PLAINTEXT')
        result[SECURITY_PROTOCOL_CFG] = security_protocol
        if security_protocol == 'SASL_PLAINTEXT' or security_protocol == 'SASL_SSL':
            result[SASL_MECHANISM_CFG] = data.get(SASL_MECHANISM_CFG, 'SCRAM-SHA-512')
            result[SASL_JAAS_CONFIG_CFG] = jaas_config_make(data['sasl.username'], data['sasl.password'])
        if security_protocol == 'SASL_SSL' and data.get(SSL_TRUSTSTORE_CERTIFICATES, ''):
            result['ssl.truststore.type'] = 'PEM'
            result[SSL_TRUSTSTORE_CERTIFICATES] = data[SSL_TRUSTSTORE_CERTIFICATES].replace(r'\n', '\n')
    return result


def jaas_config_make(username, password):
    """
    :param username: Kafka user's name.
    :param password: Kafka user's password.
    :return: JAAS-config that include given username and password.
    """
    return (
        'org.apache.kafka.common.security.scram.ScramLoginModule'
        + ' required username=\"{username}\" password=\"{password}\";'.format(username=username, password=password)
    )


def is_yandex_fqdn(fqdn):
    """
    Function check if given fqdn belongs to Yandex Cloud.
    :param fqdn: given fqdn, has 'host:port' shape.
    :return: True - if given fqdn belongs to Yandex Cloud.
             False - otherwise.
    """
    parts = fqdn.split(':')
    host = parts[0]
    for ya_suff in YANDEX_CLOUD_ENV_SUFFIXES:
        if host.endswith(ya_suff):
            return True
    return False


def is_yandex_cluster(bootstrap_servers):
    """
    Check if cluster belongs to Yandex Cloud by its 'bootstrap_servers' property.
    :param bootstrap_servers: line of fqdn's, has format 'fqdn1,fqdn2,...,fqdnN'
    :return: True - if cluster belongs to Yandex Cloud.
             False - otherwise.
    """
    for fqdn in bootstrap_servers.split(','):
        if not is_yandex_fqdn(fqdn):
            return False
    return True


def validate_connector_config(class_name, config):
    """
    Validation of connector's config with built-in REST-interface of Kafka Connect.
    :param class_name: name of the connector's Java-class.
           For example 'MirrorSourceConnector'.
    :param config: given config of connector that we want to submit.
    """
    endpoint = '/connector-plugins/' + class_name + '/config/validate'
    response = make_request(endpoint=endpoint, method=PUT_METHOD, headers=JSON_HEADERS, json=config)
    data = response.json()
    if data.get('error_count', 0) == 0:
        return
    errors = []
    for conf in data.get('configs', []):
        value = conf.get('value', {})
        name_prefix = value['name'] + ': ' if value.get('name') else ''
        for value_error in value.get('errors', []):
            errors.append(name_prefix + str(value_error))
    raise ValueError('\n'.join(errors))


def check_connection_host_port(host, port):
    """
    Check connection to fqdn using socket. Fqdn has format 'host:port'.
    :param host: host of the fqdn.
    :param port: port of the fqdn.
    :return: True - if socket connection to fqdn was successfully established.
             False - otherwise.
    """
    try:
        # python 3
        connection_refused = ConnectionRefusedError
    except NameError:
        # python 2
        connection_refused = socket.error
    try:
        sock = socket.create_connection((host, port), 3)
        sock.close()
        return True
    except (connection_refused, socket.timeout):
        return False


def test_connection_to_kafka_host(bootstrap_server):
    """
    Check connection to fqdn.
    :param bootstrap_server: fqdn that has format 'host:port'.
    :return: True - if connection to fqdn was successfully established.
             False - otherwise.
    """
    parts = bootstrap_server.split(':')
    host = parts[0]
    port = int(parts[1])
    return check_connection_host_port(host, port)


def test_connection_to_external_cluster(bootstrap_servers):
    """
    Check if external cluster, that we want to connect to is available.
    We determine that external cluster is available only if it has
    at least one available broker.
    :return: True - if cluster available.
             False - otherwise.
    """
    for host in bootstrap_servers.split(','):
        if not test_connection_to_kafka_host(host):
            raise ValueError('No connection to external broker: ' + host)


def parse_cluster_connection_config(data):
    """
    Parse all credentials to connect to Kafka cluster. Invoke func of test connection
    to cluster, if it is external cluster. Also raise exception if cluster type is invalid.
    Credentials that will return:
        - 'alias'
        - 'bootstrap.servers'
        - 'security.protocol'
        - 'sasl.mechanism' (optional, for SASL_PLAINTEXT ot SASL_SSL security.protocols)
        - 'sasl.jaas.config' (optional, for SASL_PLAINTEXT ot SASL_SSL security.protocols)
        - 'ssl.truststore.location' (optional, for SASL_SSL security.protocol)
        - 'ssl.truststore.password' (optional, for SASL_SSL security.protocol)
    :param data: incoming dictionary of cluster-connection properties.
    :return: Dictionary of credentials.
    """
    cluster_type = data['type']
    if cluster_type != CLUSTER_TYPE_THIS and cluster_type != CLUSTER_TYPE_EXTERNAL:
        raise CommandExecutionError(
            "Valid values of ClusterConnection.Type are \"{this}\" and \"{external}\".".format(
                this=CLUSTER_TYPE_THIS, external=CLUSTER_TYPE_EXTERNAL
            )
        )
    config = {'alias': data['alias']}
    if cluster_type == CLUSTER_TYPE_THIS:
        connection_config = this_cluster_connection_config()
    else:
        connection_config = external_cluster_connection_config(data)
    config.update(connection_config)
    return config


def parse_mm_cluster_connection_config(dest, data):
    """
    Only for MirrorMaker Kafka-connector!
    Parse all credentials to connect to cluster and convert it to correct
    format of JSON connector's properties.
    :param dest: 'source' - if this cluster is source for MirrorMaker.
                 'target' - if this cluster is target for MirrorMaker.
    :param data: given config to parse.
    :return: config for cluster connection of one of MirrorMaker clusters
             with correct name of properties to register connector.
    """
    prefix = dest + '.cluster.'
    creds = parse_cluster_connection_config(data)
    config = {}
    for key in creds:
        config[prefix + key] = creds[key]
    return config


def parse_mirrormaker(data):
    """
    :param data: given config to parse.
    :return: config of MirrorMaker connector ready to submit to Kafka Worker.
    """
    max_replicas = __salt__['mdb_kafka.default_replication_factor']()
    config = {
        'topics': data['topics'],
        'connector.class': 'MirrorSourceConnector',
        'emit.checkpoints.enabled': 'false',
        'offset-syncs.topic.replication.factor': max_replicas,
        'replication.factor': parse_mirrormaker_replication_factor(config=data, default=max_replicas),
    }
    config.update(parse_mm_cluster_connection_config('source', data['source']))
    config.update(parse_mm_cluster_connection_config('target', data['target']))
    return config


def parse_config_for_keys(data, keys):
    config = {}
    for key in keys:
        if key in data and data[key] != "":
            config[key] = data[key]
    return config


def parse_aws_credentials_external(data):
    """
    :param data: given config to parse.
    :return: Credentials that allow to connect to external s3-storage.
    """
    config = {
        'aws.access.key.id': data['access.key.id'],
        'aws.secret.access.key': data['secret.access.key'],
        'aws.s3.endpoint': data['endpoint'],
    }
    if 'region' in data and data['region'] != "":
        config['aws.s3.region'] = data['region']
    return config


def parse_aws_credentials(data):
    """
    :param data: given config to parse.
    :return: Credentials that allow to connect to s3-storage.
    """
    config = {
        'aws.s3.bucket.name': data['bucket.name'],
    }
    if data['type'] == 's3_external':
        config.update(parse_aws_credentials_external(data=data))
    return config


def parse_s3_sink_connector(data):
    """
    :param data: given config to parse.
    :return: config of S3-Sink connector ready to submit to Kafka Worker.
    """
    config = {'connector.class': 'io.aiven.kafka.connect.s3.AivenKafkaConnectS3SinkConnector', 'topics': data['topics']}
    config.update(parse_aws_credentials(data['s3.connection']))
    remain_config_keys = ['file.compression.type', 'file.max.records']
    config.update(parse_config_for_keys(data, remain_config_keys))
    return config


def parse_connector_specific_config(connector_type, data):
    """
    Parse config of specific connector.
    :param connector_type: type of connector. For example: 'mirrormaker' or 's3.sink'.
    :param data: given dict to parse into specific connector's config.
    :return: config of specific connector, fields in correct format to register connector.
    """
    if connector_type == 'mirrormaker':
        config = parse_mirrormaker(data)
    elif connector_type == 's3.sink':
        config = parse_s3_sink_connector(data)
    else:
        raise CommandExecutionError("Not valid type of connector: " + connector_type + ".")
    return config


def make_plain_connector_config(connector_data):
    """
    Parse given nested data of connector into plain config.
    :param connector_data: nested data of the connector.
    :return: plain config of the connector.
    """
    connector_data = dict_keys_from_grpc_to_json(data=connector_data)
    config = {'name': connector_data['name'], TASKS_MAX_CFG: parse_tasks_max(connector_data)}
    connector_type = cfg_name_from_grpc_to_json(connector_data['type'])
    if connector_type in connector_data:
        connector_specific_config = parse_connector_specific_config(connector_type, connector_data[connector_type])
        config.update(connector_specific_config)
    else:
        raise CommandExecutionError("Not valid type of connector: " + connector_type + ".")
    if 'properties' in connector_data:
        config.update(connector_data['properties'])
    return config


def get_connector_status(name):
    """
    :param name: name of the connector.
    :return: status of the connector. It has 4 valid values:
                - 'UNASSIGNED'
                - 'RUNNING'
                - 'PAUSED'
                - 'FAILED'
    """
    endpoint = '/connectors/' + escape_connector_name(name) + '/status'
    response = make_request(endpoint=endpoint, method=GET_METHOD)
    response_data = response.json()
    return response_data['connector']['state']


def connector_pause(name):
    """
    Pause the connector using REST-endpoint of Kafka Connect.
    After that operation connector will accept status 'PAUSED'.
    :param name: name of the connector.
    """
    endpoint = '/connectors/' + escape_connector_name(name) + '/pause'
    make_request(endpoint=endpoint, method=PUT_METHOD)


def connector_resume(name):
    """
    Resume the connector using REST-endpoint of Kafka Connect.
    After that operation connector will accept status 'RUNNING'.
    :param name: name of the connector.
    """
    endpoint = '/connectors/' + escape_connector_name(name) + '/resume'
    make_request(endpoint=endpoint, method=PUT_METHOD)


def parse_mirrormaker_replication_factor(config, default):
    """
    Parse replication.factor property of mirrormaker connector from its config.
    :param config: config of mirrormaker connector
    :param default: default value of replication.factor, if config of mirrormaker
                    doesn't contains it.
    :return: parsed value of replication.factor property for mirrormaker connector.
    """
    result = config.get('replication.factor', default)
    brokers_count = __salt__['mdb_kafka.count_of_brokers']()
    if result < 1 or result > brokers_count:
        raise CommandExecutionError(
            "Invalid value of replication.factor for mirrormaker: {rf}. Must be between 1 and {max_rf}".format(
                rf=result, max_rf=brokers_count
            )
        )
    return result


def cfg_name_from_grpc_to_json(cfg):
    return cfg.replace('_', '.')


def dict_keys_from_grpc_to_json(data):
    result = {}
    for key, value in data.items():
        key_temp = cfg_name_from_grpc_to_json(key)
        value_temp = value
        if isinstance(value, dict):
            value_temp = dict_keys_from_grpc_to_json(data=value)
        result[key_temp] = value_temp
    return result
