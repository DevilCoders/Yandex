# -*- coding: utf-8 -*-
"""
Apache Kafka utility functions for MDB
"""

import os
import re
import string
import time
import hashlib
import json
import logging
import socket

from collections import namedtuple
from contextlib import closing

try:
    from salt.exceptions import CommandExecutionError, TimeoutError
    from salt.utils.stringutils import to_bytes
except ImportError as e:
    # salt library is not available in tests
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils
    from cloud.mdb.salt_tests.common.arc_utils import to_bytes

    arc_utils.raise_if_not_arcadia(e)

try:
    from confluent_kafka import KafkaException, KafkaError
    from confluent_kafka.admin import AdminClient, ConfigResource, ConfigSource, NewTopic, NewPartitions

    HAS_KAFKA_LIB = True
except ImportError:
    HAS_KAFKA_LIB = False

try:
    from py4j.java_gateway import JavaGateway
    from py4j.java_collections import ListConverter

    HAS_PY4J_LIB = True
except ImportError:
    HAS_PY4J_LIB = False

__salt__ = {}

log = logging.getLogger(__name__)

API_TIMEOUT = 60 * 1000
REASSIGN_FILEPATH = '/opt/kafka/reassign.json'
ADMIN_CONTEXT = None

AdminContext = namedtuple('AdminContext', ['gateway', 'api', 'pkg'])
Permission = namedtuple('Permission', ['topic_name', 'role', 'group', 'host'])


def __virtual__():
    return True


def jvm_xmx_gb():
    Gb = 1024 * 1024 * 1024
    mem_limit = __salt__['pillar.get']('data:dbaas:flavor:memory_limit', 1 * Gb)
    heap_limits = [
        (1 * Gb, 1 * Gb),
        (8 * Gb, 2 * Gb),
        (16 * Gb, 6 * Gb),
        (48 * Gb, 12 * Gb),
        (128 * Gb, 24 * Gb),
    ]
    curr_limit = 1 * Gb
    for mem_size, limit in heap_limits:
        if mem_size > mem_limit:
            break
        curr_limit = limit
    return int(curr_limit / Gb)


def check_name(name, object_type):
    if not re.match(r"^[\-\w_.]+$", name):
        raise CommandExecutionError("Bad {} name {}".format(name, object_type))


def count_of_brokers():
    brokers = __salt__['pillar.get']('data:kafka:nodes', {})
    return len(brokers.items())


def default_replication_factor(threshold=3):
    """
    Replication factor for service topic.
    """
    brokers_count = count_of_brokers()
    if brokers_count >= threshold:
        return threshold
    else:
        return 1


def count_of_brokers_with_threshold(threshold=3):
    """
    Replication factor for service topic.
    """
    brokers_count = count_of_brokers()
    if brokers_count >= threshold:
        return threshold
    else:
        return 1


def check_topic_pattern(pattern):
    if not pattern:
        raise CommandExecutionError("Topic name can not be empty")
    if not re.match(r"^[\-\w_.]*\*?$", pattern):
        raise CommandExecutionError("Bad topic pattern {}".format(pattern))


def check_group_name(name):
    if name == '*':
        return
    check_name(name, 'group')


def making_meta_changes():
    """
    Check if this host is broker to make metadata changes
    """
    fqdn = __salt__['grains.get']('fqdn')
    brokers = __salt__['pillar.get']('data:kafka:nodes')
    broker_ids = set(broker.get('id') for broker in brokers.values())
    # Choose main_broker by specified ID in pillar, if set.
    # Else, take minimal ID from existing brokers.
    main_broker_id = __salt__['pillar.get']('data:kafka:main_broker_id')
    if main_broker_id not in broker_ids:
        main_broker_id = min(broker_ids)

    return fqdn in brokers and brokers[fqdn].get('id') == main_broker_id


def kafka_connect_cluster_brokers_list():
    """
    Return list of Kafka brokers.
    """
    brokers = __salt__['pillar.get']('data:kafka:nodes', {})
    kafka_nodes = []
    for broker_fqdn, broker in brokers.items():
        kafka_nodes.append(broker_fqdn + ':9091')
    return ','.join(kafka_nodes)


def kafka_fqdns():
    """
    Return Kafka fqdn's,
    separated by ,
    """
    brokers = __salt__['pillar.get']('data:kafka:nodes', {})
    kafka_nodes = []
    for broker_fqdn, broker in brokers.items():
        kafka_nodes.append(broker_fqdn)
    return ','.join(kafka_nodes)


def get_ipv6_address(fqdn, port):
    addrs = socket.getaddrinfo(fqdn, port, socket.AF_INET6, socket.SOCK_STREAM, socket.IPPROTO_TCP)
    addr = addrs[0]
    return addr[4][0]


def ssl_cipher_suites():
    suites = __salt__['pillar.get']('data:kafka:config:ssl.cipher.suites', {})
    return ','.join(suites)


def zk_connect():
    """
    Return list of ZooKeeper's 'host:port'
    """
    if not __salt__['pillar.get']('data:kafka:has_zk_subcluster', False):
        return __salt__['pillar.get']('data:kafka:zk_connect', 'localhost:2181')
    zk_nodes = []
    for node in __salt__['pillar.get']('data:zk:nodes', {}):
        if __salt__['dbaas.is_aws']():
            node = get_ipv6_address(node, 2181)
        zk_nodes.append(node + ':2181')
    return ','.join(zk_nodes)


def zk_fqdns():
    """
    Return ZooKeeper fqdn's,
    separated by ,
    """
    if not __salt__['pillar.get']('data:kafka:has_zk_subcluster', False):
        return ''
    zk_nodes = []
    for node in __salt__['pillar.get']('data:zk:nodes', {}):
        zk_nodes.append(node)
    return ','.join(zk_nodes)


def jmx_password():
    if __salt__['pillar.get']('data:running_on_template_machine', False):
        return 'nopassword'
    chars = string.ascii_letters + string.digits
    return __salt__['grains.get_or_set_hash']('jmx_password', 14, chars)


def monitor_password():
    if __salt__['pillar.get']('data:running_on_template_machine', False):
        return 'nopassword'
    from_pillar = __salt__['pillar.get']('data:kafka:monitor_password')
    if from_pillar:
        return from_pillar
    chars = string.ascii_letters + string.digits
    return __salt__['grains.get_or_set_hash']('monitor_password', 14, chars)


def create_socket():
    dbaas_vtype = __salt__['pillar.get']('data:dbaas:vtype')
    if dbaas_vtype == 'compute':
        sock = socket.socket()
    else:
        sock = socket.socket(socket.AF_INET6)
    return sock


def check_zk_host(host):
    with closing(create_socket()) as sock:
        sock.settimeout(3)
        sock.connect((host, 2181))
        sock.send(to_bytes('ruok'))
        resp = sock.recv(4)
        if resp == to_bytes('imok'):
            return True
        return False


def zk_ok():
    """
    Check that all zk hosts is ok
    """
    if not __salt__['pillar.get']('data:kafka:has_zk_subcluster'):
        return check_zk_host('localhost')
    for node in __salt__['pillar.get']('data:zk:nodes', {}):
        if not check_zk_host(node):
            return False
    return True


def wait_for_zk(wait_timeout=600):
    deadline = time.time() + wait_timeout
    while time.time() <= deadline:
        try:
            if zk_ok():
                return
        except Exception as e:
            log.debug('Failed to connect to ZK: %s', repr(e))
            time.sleep(5)
    raise TimeoutError('failed to connect to ZK within timeout')


def _config_command(param_cmds, **kwargs):
    if __salt__['pillar.get']('data:kafka:use_plain_sasl'):
        cmds = [
            '/opt/kafka/bin/kafka-configs.sh',
            '--bootstrap-server',
            kafka_connect_cluster_brokers_list(),
            '--command-config',
            '/etc/kafka/connect.properties',
        ]
    else:
        zk_conn = zk_connect()
        if not zk_conn:
            raise CommandExecutionError(param_cmds + '\nNo ZK connect string')
        cmds = [
            '/opt/kafka/bin/kafka-configs.sh',
            '--zookeeper',
            zk_conn,
        ]

    cmds.extend(param_cmds)
    cmd = ' '.join(cmds)
    cmd_kwargs = {
        'ignore_retcode': True,
        'python_shell': True,
    }
    cmd_kwargs.update(kwargs)
    result = __salt__['cmd.run_all'](cmd, **cmd_kwargs)

    if result['retcode'] != 0:
        raise CommandExecutionError(cmd + '\nstdout:\n' + result['stdout'] + '\nstderr:\n' + result['stderr'])
    return result['stdout']


def list_users():
    cmds = ['--describe', '--entity-type', 'users']
    out = _config_command(cmds)
    return re.findall(r"(?:C|c)onfigs for user-principal '([\-\w]*)'", out)


def calc_password_hash(password):
    password_salt = __salt__['grains.get_or_set_hash']('password_salt')
    h = hashlib.sha256()
    h.update(to_bytes(password_salt))
    h.update(to_bytes(password))
    return h.hexdigest()


def user_password_matches(name, password):
    # On template machine do nothing
    if __salt__['pillar.get']('data:running_on_template_machine', False):
        return True
    grains_key = 'passwords_' + name
    hashed_password = __salt__['grains.get'](grains_key)
    if hashed_password and hashed_password == calc_password_hash(password):
        return True
    return False


def set_user_password_hash(name, password):
    grains_key = 'passwords_' + name
    __salt__['grains.set'](grains_key, calc_password_hash(password))


def create_user_auth(name, password):
    check_name(name, 'user')
    if "'" in password:
        raise CommandExecutionError("Special symbols in password")

    cmds = [
        '--alter',
        '--entity-type',
        'users',
        '--add-config',
        '"SCRAM-SHA-512=[password=${PASS}]"',
        '--entity-name',
        name,
    ]
    stdout = _config_command(cmds, env={'PASS': password})
    # If no exception - user created successful
    set_user_password_hash(name, password)
    return stdout


def delete_user_auth(name):
    check_name(name, 'user')
    cmds = ['--alter', '--entity-type', 'users', '--delete-config', "'SCRAM-SHA-512'", '--entity-name', name]
    return _config_command(cmds)


def _remove_kafka_acls_help(output):
    match = re.compile(r'^(.*)(\nOption\s+Description.*)$', re.DOTALL).match(output)
    if not match:
        return output
    return match.group(1)


def _acl_command(param_cmds, **kwargs):
    if __salt__['pillar.get']('data:kafka:use_plain_sasl'):
        cmds = [
            '/opt/kafka/bin/kafka-acls.sh',
            '--bootstrap-server',
            kafka_connect_cluster_brokers_list(),
            '--command-config',
            '/etc/kafka/connect.properties',
        ]
    else:
        zk_conn = zk_connect()
        if not zk_conn:
            raise CommandExecutionError('No ZK connect string')
        cmds = [
            '/opt/kafka/bin/kafka-acls.sh',
            '--authorizer-properties',
            'zookeeper.connect=' + zk_conn,
        ]
    cmds.extend(param_cmds)
    cmd = ' '.join(cmds)
    cmd_kwargs = {
        'ignore_retcode': True,
        'python_shell': True,
    }
    cmd_kwargs.update(kwargs)
    result = __salt__['cmd.run_all'](cmd, **cmd_kwargs)

    if result['retcode'] != 0:
        stdout = _remove_kafka_acls_help(result['stdout'].decode("utf-8"))
        stderr = _remove_kafka_acls_help(result['stderr'].decode("utf-8"))

        raise CommandExecutionError(cmd + '\nstdout:\n' + stdout + '\nstderr:\n' + stderr)
    return result['stdout']


def _generate_topic_block(topic_pattern):
    '''
    Topic wildcard pattern resolve
    '''
    check_topic_pattern(topic_pattern)
    if topic_pattern == '*':
        topic_block = ['--topic', '"*"']
    elif topic_pattern.endswith("*"):
        topic_block = [
            '--topic',
            topic_pattern[:-1],
            '--resource-pattern-type prefixed',
        ]
    else:
        topic_block = ['--topic', topic_pattern]
    return topic_block


def _add_topic_permission(username, topic_pattern, operation):
    cmds = ['--add', '--force', '--allow-principal', 'User:' + username, '--operation ' + operation]
    cmds += _generate_topic_block(topic_pattern)
    _acl_command(cmds)


def _add_cluster_permission(username, operation):
    cmds = ['--add', '--force', '--cluster', '--allow-principal', 'User:' + username, '--operation ' + operation]
    _acl_command(cmds)


def grant_common_consumer_permissions(username):
    allow_group_operation(username, '*', 'READ')


def grant_role(username, topic_pattern, role, group=None):
    if not group:
        group = '*'
    check_name(username, 'user')
    check_group_name(group)

    if role == 'admin':
        grant_role(username, topic_pattern, 'producer', group)
        grant_role(username, topic_pattern, 'consumer', group)
        kafka_version = __salt__['pillar.get']('data:kafka:version')
        if kafka_version != '2.1':
            _add_topic_permission(username, topic_pattern, 'Alter')
        _add_topic_permission(username, topic_pattern, 'AlterConfigs')
        _add_topic_permission(username, topic_pattern, 'Delete')
        _add_cluster_permission(username, 'Create')
        _add_cluster_permission(username, 'Describe')
        _add_cluster_permission(username, 'DescribeConfigs')
        return

    cmds = [
        '--add',
        '--force',
        '--' + role,
        '--allow-principal',
        'User:' + username,
        '--group',
        '"' + group + '"',
    ]
    cmds += _generate_topic_block(topic_pattern)
    if role == 'producer':
        cmds += ['--idempotent']
    _acl_command(cmds)

    if role == 'producer':
        allow_group_operation(username, '*', 'READ')
        allow_group_operation(username, '*', 'DESCRIBE')
        # Add transaction write
        cmds = [
            '--add',
            '--force',
            '--allow-principal',
            'User:' + username,
            '--transactional-id "*"',
            '--operation Write',
        ]
        _acl_command(cmds)

    _add_cluster_permission(username, 'Describe')
    _add_topic_permission(username, topic_pattern, 'DescribeConfigs')


def grant_monitor_role(username):
    allow_group_operation(username, '*', 'READ')
    allow_group_operation(username, '*', 'DESCRIBE')
    _add_topic_permission(username, '*', 'DESCRIBE')
    _add_cluster_permission(username, 'DESCRIBE')


def _remove_topic_permission(username, topic_pattern, operation):
    cmds = ['--remove', '--force', '--allow-principal', 'User:' + username, '--operation ' + operation]
    cmds += _generate_topic_block(topic_pattern)
    _acl_command(cmds)


def _remove_cluster_permission(username, operation):
    cmds = ['--remove', '--force', '--cluster', '--allow-principal', 'User:' + username, '--operation ' + operation]
    _acl_command(cmds)


def deny_role(username, topic_pattern, role, group=None):
    if not group:
        group = '*'
    check_name(username, 'user')
    check_group_name(group)

    if role == 'admin':
        deny_role(username, topic_pattern, 'producer', group)
        deny_role(username, topic_pattern, 'consumer', group)
        _remove_topic_permission(username, topic_pattern, 'Alter')
        _remove_topic_permission(username, topic_pattern, 'AlterConfigs')
        _remove_topic_permission(username, topic_pattern, 'Delete')
        _remove_cluster_permission(username, 'Create')
        _remove_cluster_permission(username, 'Describe')
        _remove_cluster_permission(username, 'DescribeConfigs')
        return

    _remove_topic_permission(username, topic_pattern, 'DescribeConfigs')

    cmds = [
        '--remove',
        '--force',
        '--' + role,
        '--allow-principal',
        'User:' + username,
        '--group',
        '"' + group + '"',
    ]
    cmds += _generate_topic_block(topic_pattern)
    return _acl_command(cmds)


def _operation_in_pascal_case(operation):
    if operation.upper() != operation:
        # Here we expect operation to be something like DESCRIBE_CONFIGS
        # If it is not, do nothing
        return operation
    return ''.join(word.title() for word in operation.lower().split('_'))


def deny_topic_operation(username, topic_pattern, operation):
    check_name(username, 'user')
    check_name(operation, 'operation')

    cmds = [
        '--remove',
        '--force',
        '--operation',
        _operation_in_pascal_case(operation),
        '--allow-principal',
        'User:' + username,
    ]
    cmds += _generate_topic_block(topic_pattern)
    return _acl_command(cmds)


def allow_group_operation(username, group, operation):
    check_name(username, 'user')
    check_group_name(group)

    cmds = [
        '--add',
        '--force',
        '--operation',
        operation,
        '--allow-principal',
        'User:' + username,
        '--group',
        '"' + group + '"',
    ]
    return _acl_command(cmds)


def deny_group_operation(username, group, operation):
    check_name(username, 'user')
    check_group_name(group)

    cmds = [
        '--remove',
        '--force',
        '--operation',
        operation,
        '--allow-principal',
        'User:' + username,
        '--group',
        '"' + group + '"',
    ]
    return _acl_command(cmds)


def operations_to_roles(operations):
    result = set()
    for op in operations:
        if op.lower() == 'read':
            result.add('consumer')
        if op.lower() == 'write':
            result.add('producer')
        # 'ALTER_CONFIGS' - in kafka 2.6+, 'AlterConfigs' - in kafka 2.1
        if op.lower() == 'alter_configs' or op.lower() == 'alterconfigs':
            # admin role includes prducer and consumer roles
            return ['admin']
    return list(result)


def user_topic_roles(name, return_operations=False):
    check_name(name, 'user')
    cmds = [
        '--list',
        '--principal',
        'User:' + name,
    ]
    out = _acl_command(cmds)
    out_parts = out.split('\n\n')
    result = {}
    for part in out_parts:
        topics = re.findall(r"Current ACLs for resource `Topic:LITERAL:(.*)`:", part)
        if not topics:
            topics = re.findall(r"`ResourcePattern\(resourceType=TOPIC, name=(.*), patternType=LITERAL\)`", part)
        if topics:
            topic = topics[0]
            operations = re.findall(r"has Allow permission for operations: (\w*) from", part)
            if not operations:
                operations = re.findall(r"operation=(\w*), permissionType=ALLOW", part)
            if not return_operations:
                result[topic] = operations_to_roles(operations)
            else:
                result[topic] = operations
    return result


def user_topic_prefixes_roles(name, return_operations=False):
    check_name(name, 'user')
    cmds = [
        '--list',
        '--principal',
        'User:' + name,
    ]
    out = _acl_command(cmds)
    topic_patterns = re.findall(r"`ResourcePattern\(resourceType=TOPIC, name=([\-\w]*), patternType=PREFIXED\)`", out)
    result = {}
    for topic_pattern in topic_patterns:
        topic_cmds = list(cmds)
        topic_cmds += _generate_topic_block(topic_pattern + '*')
        out = _acl_command(topic_cmds)
        operations = re.findall(r"has Allow permission for operations: (\w*) from", out)
        if not operations:
            operations = re.findall(r"operation=(\w*), permissionType=ALLOW", out)
        if not return_operations:
            result[topic_pattern] = operations_to_roles(operations)
        else:
            result[topic_pattern] = operations
    return result


def user_group_operations(name):
    check_name(name, 'user')
    cmds = [
        '--list',
        '--principal',
        'User:' + name,
    ]
    out = _acl_command(cmds)
    groups = re.findall(r"Current ACLs for resource `Group:LITERAL:(.*)`:", out)
    result = {}
    for group in groups:
        group_cmds = list(cmds)
        group_cmds += ['--group', '"' + group + '"']
        out = _acl_command(group_cmds)
        operations = re.findall(r"has Allow permission for operations: (\w*) from", out)
        result[group] = operations
    return result


def clean_user_roles(name):
    check_name(name, 'user')
    operations = user_topic_roles(name, return_operations=True)
    for topic, topic_operations in operations.items():
        for topic_operation in topic_operations:
            deny_topic_operation(name, topic, topic_operation)
    operations = user_topic_prefixes_roles(name, return_operations=True)
    for topic_prefix, topic_operations in operations.items():
        for topic_operation in topic_operations:
            deny_topic_operation(name, topic_prefix + '*', topic_operation)
    operations = user_group_operations(name)
    for group, group_operations in operations.items():
        for group_operation in group_operations:
            deny_group_operation(name, group, group_operation)


def _get_admin_client():
    if not HAS_KAFKA_LIB:
        raise CommandExecutionError('No Kafka lib available')
    fqdn = __salt__['grains.get']('fqdn')
    if not fqdn:
        raise CommandExecutionError('No FQDN available')
    params = {
        'bootstrap.servers': fqdn + ':9091',
        'request.timeout.ms': API_TIMEOUT,
        'security.protocol': 'SASL_SSL',
        'ssl.ca.location': '/etc/kafka/ssl/cert-ca.pem',
        'sasl.mechanism': 'SCRAM-SHA-512',
        'sasl.username': 'mdb_admin',
        'sasl.password': __salt__['pillar.get']('data:kafka:admin_password'),
    }
    if __salt__['dbaas.is_aws']():
        params['broker.address.family'] = 'v6'
    return AdminClient(params)


def list_topics():
    adm = _get_admin_client()
    md = adm.list_topics(timeout=API_TIMEOUT / 1000.0)
    return list(md.topics.keys())


def get_topic(name):
    adm = _get_admin_client()
    meta = adm.list_topics(timeout=API_TIMEOUT / 1000.0)
    return meta.topics.get(name)


def get_topic_replicas(name):
    topic = get_topic(name)
    res = {}
    for num, meta in topic.partitions.items():
        res[num] = meta.replicas
    return res


def _reassign_command(param_cmds, **kwargs):
    zk_conn = zk_connect()
    if not zk_conn:
        raise CommandExecutionError(param_cmds + '\nNo ZK connect string')

    cmds = [
        '/opt/kafka/bin/kafka-reassign-partitions.sh',
        '--bootstrap-server',
        kafka_connect_cluster_brokers_list(),
        '--command-config',
        '/etc/kafka/connect.properties',
    ]
    cmds.extend(param_cmds)
    cmd = ' '.join(cmds)
    cmd_kwargs = {
        'ignore_retcode': True,
        'python_shell': True,
    }
    cmd_kwargs.update(kwargs)
    result = __salt__['cmd.run_all'](cmd, **cmd_kwargs)

    if result['retcode'] != 0:
        raise CommandExecutionError(cmd + '\nstdout:\n' + result['stdout'] + '\nstderr:\n' + result['stderr'])
    return result['stdout']


def reassign_status():
    cmds = ['--verify', '--reassignment-json-file', REASSIGN_FILEPATH]
    return _reassign_command(cmds)


def check_reassign_status():
    out = reassign_status()
    in_progress = False
    has_failed = False
    for match in re.findall(r"Reassignment of partition ([\-\w]*) (.*)($|\n)", out):
        status = match[1]
        if status == 'failed':
            has_failed = True
            continue
        if status in ['completed successfully', 'is complete.']:
            continue
        in_progress = True
    return in_progress, has_failed


def _wait_reassign():
    while True:
        in_progress, has_failed = check_reassign_status()
        if in_progress:
            time.sleep(30)
            continue
        if has_failed:
            raise CommandExecutionError("Failed to reassign partition")
        return


def reassignment_file_content(name, new_partitions):
    partitions_params = []
    for partition, replicas in new_partitions.items():
        partitions_params.append(
            {
                'topic': name,
                'partition': partition,
                'replicas': replicas,
            }
        )
    return {'version': 1, 'partitions': partitions_params}


def reassign_partitions(name, new_partitions):
    # Ensure that previous reassign is done
    if os.path.exists(REASSIGN_FILEPATH):
        _wait_reassign()

    with open(REASSIGN_FILEPATH, 'w') as f:
        json.dump(reassignment_file_content(name, new_partitions), f)
    os.chmod(REASSIGN_FILEPATH, 644)

    cmds = ['--execute', '--reassignment-json-file', REASSIGN_FILEPATH]
    _reassign_command(cmds)

    _wait_reassign()
    # Remove reassign file if reassign is done
    os.unlink(REASSIGN_FILEPATH)


def create_topic(name, partitions, replication_factor=3, config=None):
    adm = _get_admin_client()
    config = config or {}
    res = adm.create_topics([NewTopic(name, partitions, replication_factor, config=config)])
    future = res[name]
    try:
        future.result()
        return True
    except KafkaException as e:
        err = e.args[0]
        if isinstance(err, KafkaError) and err.code() == KafkaError.TOPIC_ALREADY_EXISTS:
            return False
        else:
            raise


def delete_topic(name):
    adm = _get_admin_client()
    res = adm.delete_topics([name])
    future = res[name]
    try:
        future.result()
        return True
    except KafkaException as e:
        err = e.args[0]
        if isinstance(err, KafkaError) and err.code() == KafkaError.UNKNOWN_TOPIC_OR_PART:
            return False
        else:
            raise


def increase_topic_partitions(name, new_partitions):
    adm = _get_admin_client()
    res = adm.create_partitions([NewPartitions(name, new_partitions)])
    future = res[name]
    try:
        future.result()
        return True
    except KafkaException as e:
        err = e.args[0]
        if isinstance(err, KafkaError) and err.str().startswith('Topic already has'):
            return False
        else:
            raise


def topic_config_planned_changes(name, config, known_topic_config_properties):
    topic_config = get_topic_config(name)
    all_valid_props = set(topic_config.keys())
    invalid_props = set(config.keys()) - all_valid_props
    if invalid_props:
        raise CommandExecutionError(
            "Config for topic '{}' contains unknown properties: {}".format(name, ','.join(invalid_props))
        )
    target_props = set(config.keys())
    current_props = {k for k, v in topic_config.items() if v.source == ConfigSource.DYNAMIC_TOPIC_CONFIG.value}
    known_topic_config_properties = set(known_topic_config_properties)
    deleted_props = current_props - target_props
    if known_topic_config_properties:
        # known_topic_config_properties might be empty in two cases:
        # * before we release version of int api that sets this pillar prop
        # * if we run highstate before any pillar-changing operation
        deleted_props = deleted_props & known_topic_config_properties
    changes = {}
    for k, v in config.items():
        val = topic_config.get(k)
        # property wasn't set as dynamic for topic or its value changed
        stringified_pillar_value = str(v)
        if isinstance(v, bool):
            stringified_pillar_value = 'true' if v else 'false'
        if val.source != ConfigSource.DYNAMIC_TOPIC_CONFIG.value or str(val.value) != stringified_pillar_value:
            changes[k] = v

    target_config = config.copy()
    for k, v in topic_config.items():
        if v.source != ConfigSource.DYNAMIC_TOPIC_CONFIG.value:
            continue
        if k in target_config:
            continue
        if k in known_topic_config_properties:
            continue
        target_config[k] = v.value

    report = []
    if changes:
        report.append('changed ' + ','.join(['{}={}'.format(k, v) for k, v in changes.items()]))
    if deleted_props:
        report.append('reset ' + ','.join(deleted_props))
    return ', '.join(report), target_config


def get_topic_config(name):
    adm = _get_admin_client()
    resource = ConfigResource(ConfigResource.Type.TOPIC, name)
    result = adm.describe_configs([resource])
    future = result[resource]
    return future.result()


def alter_topic_config(name, set_config):
    adm = _get_admin_client()
    resource = ConfigResource(ConfigResource.Type.TOPIC, name, set_config=set_config)
    result = adm.alter_configs([resource])
    # wait until async operation is done
    result[resource].result()
    return True


def karapace_password_salt():
    chars = string.ascii_letters + string.digits
    return str(__salt__['grains.get_or_set_hash']('karapace_password_salt', 14, chars))


def karapace_password_hash(user, password):
    salt = karapace_password_salt()
    payload = salt + user + password
    return hashlib.sha512(to_bytes(payload)).hexdigest()


def karapace_users():
    """
    We need to make JSON structure with all users/passwords and topics where user have producer permissions
    {
        "consumer": {"password": "ConsumerPassword_hash", "topics": []}
        "producer": {"password": "ProducerPassword_hash", "topics": ["topic1", "topic2"]}
    }
    """
    result = {}
    users = __salt__['pillar.get']('data:kafka:users')
    for name, user in users.items():
        write_topics = set()
        for perm in user.get('permissions', []):
            if perm.get('role') == 'producer' or perm.get('role') == 'admin':
                write_topics.add(perm['topic_name'])
        result[name] = {'password': karapace_password_hash(name, user['password']), 'topics': list(write_topics)}
    return result


def _get_admin_context():
    global ADMIN_CONTEXT
    if ADMIN_CONTEXT:
        return ADMIN_CONTEXT

    if not HAS_PY4J_LIB:
        raise CommandExecutionError('No Kafka lib available')
    fqdn = __salt__['grains.get']('fqdn')
    if not fqdn:
        raise CommandExecutionError('No FQDN available')

    pwd = __salt__['pillar.get']('data:kafka:admin_password')
    jaas = (
        'org.apache.kafka.common.security.scram.ScramLoginModule required username="mdb_admin" password="{}";'.format(
            pwd
        )
    )
    params = {
        'bootstrap.servers': fqdn + ':9091',
        'request.timeout.ms': str(API_TIMEOUT),
        'security.protocol': 'SASL_SSL',
        'ssl.truststore.location': '/etc/kafka/ssl/server.truststore.jks',
        'ssl.truststore.password': pwd,
        'sasl.jaas.config': jaas,
        'sasl.mechanism': 'SCRAM-SHA-512',
    }
    gateway = JavaGateway()
    props = gateway.jvm.java.util.Properties()
    for name, value in params.items():
        props.setProperty(name, value)
    api = gateway.entry_point.admin(props)
    ADMIN_CONTEXT = AdminContext(gateway, api, gateway.jvm.org.apache.kafka)
    return ADMIN_CONTEXT


def convert_list(context, python_list):
    """
    Convert python list to java list
    """
    return ListConverter().convert(python_list, context.gateway._gateway_client)


def list_users_py4j():
    context = _get_admin_context()
    return list(context.api.describeUserScramCredentials().users().get())


def create_user_auth_py4j(name, password):
    context = _get_admin_context()
    UserScramCredentialUpsertion = context.pkg.clients.admin.UserScramCredentialUpsertion
    ScramCredentialInfo = context.pkg.clients.admin.ScramCredentialInfo
    SCRAM_SHA_512 = context.pkg.clients.admin.ScramMechanism.SCRAM_SHA_512

    credentialInfo = ScramCredentialInfo(SCRAM_SHA_512, 4096)
    upsertion = UserScramCredentialUpsertion(name, credentialInfo, password)
    upsertions = convert_list(context, [upsertion])
    resp = context.api.alterUserScramCredentials(upsertions)
    resp.all().get()
    set_user_password_hash(name, password)


def delete_user_auth_py4j(name):
    context = _get_admin_context()
    UserScramCredentialDeletion = context.pkg.clients.admin.UserScramCredentialDeletion
    SCRAM_SHA_512 = context.pkg.clients.admin.ScramMechanism.SCRAM_SHA_512

    deletion = UserScramCredentialDeletion(name, SCRAM_SHA_512)
    resp = context.api.alterUserScramCredentials(convert_list(context, [deletion]))
    resp.all().get()


def sync_acls(user, permission_dicts, test=True):
    return _sync_acls(user, permission_dicts, test, delete=True)


def add_permissions(user, permission_dicts, test=True):
    return _sync_acls(user, permission_dicts, test, delete=False)


def clean_user_roles_py4j(username):
    bindings = set(current_acls(username))
    if bindings:
        context = _get_admin_context()
        binding_filters = [binding_to_binding_filter(context, binding) for binding in bindings]
        context.api.deleteAcls(convert_list(context, binding_filters)).all().get()


def _sync_acls(user, permission_dicts, test=True, delete=True):
    permissions = [
        Permission(perm['topic_name'], perm['role'], perm.get('group') or '*', perm.get('host') or '*')
        for perm in permission_dicts
    ]
    target = set()
    for permission in permissions:
        target |= permission_to_acls(user, permission)

    current = set(current_acls(user))
    to_add = target - current
    to_delete = current - target if delete else set()

    report = build_acls_report(to_add, to_delete)

    if not test:
        context = _get_admin_context()

        if to_add:
            context.api.createAcls(convert_list(context, list(to_add))).all().get()

        if to_delete:
            binding_filters = [binding_to_binding_filter(context, binding) for binding in to_delete]
            context.api.deleteAcls(convert_list(context, binding_filters)).all().get()

    return report


def current_acls(user):
    context = _get_admin_context()

    AclBindingFilter = context.pkg.common.acl.AclBindingFilter
    ResourcePatternFilter = context.pkg.common.resource.ResourcePatternFilter

    acl_filter = AclBindingFilter(ResourcePatternFilter.ANY, _acl_filter_by_user(user))
    resp = context.api.describeAcls(acl_filter)
    return resp.values().get()


def _acl_filter_by_user(user):
    context = _get_admin_context()
    AclOperation = context.pkg.common.acl.AclOperation
    AclPermissionType = context.pkg.common.acl.AclPermissionType
    AccessControlEntryFilter = context.pkg.common.acl.AccessControlEntryFilter

    principal = 'User:' + str(user)
    host = '*'
    return AccessControlEntryFilter(principal, host, AclOperation.ANY, AclPermissionType.ANY)


def pattern_and_pattern_type(context, name):
    PatternType = context.pkg.common.resource.PatternType
    if name.endswith("*") and name != "*":
        return name[:-1], PatternType.PREFIXED
    return name, PatternType.LITERAL


def permission_to_acls(username, permission):
    context = _get_admin_context()
    AclBinding = context.pkg.common.acl.AclBinding
    AclOperation = context.pkg.common.acl.AclOperation
    AclPermissionType = context.pkg.common.acl.AclPermissionType
    AccessControlEntry = context.pkg.common.acl.AccessControlEntry
    ResourcePattern = context.pkg.common.resource.ResourcePattern
    PatternType = context.pkg.common.resource.PatternType
    ResourceType = context.pkg.common.resource.ResourceType

    pairs = [(ResourceType.CLUSTER, AclOperation.DESCRIBE), (ResourceType.TOPIC, AclOperation.DESCRIBE_CONFIGS)]
    if permission.role == 'consumer' or permission.role == 'admin':
        pairs.append((ResourceType.TOPIC, AclOperation.READ))
        pairs.append((ResourceType.TOPIC, AclOperation.DESCRIBE))
        pairs.append((ResourceType.GROUP, AclOperation.READ))
    if permission.role == 'producer' or permission.role == 'admin':
        pairs.append((ResourceType.TOPIC, AclOperation.WRITE))
        pairs.append((ResourceType.TOPIC, AclOperation.DESCRIBE))
        pairs.append((ResourceType.TOPIC, AclOperation.CREATE))
        pairs.append((ResourceType.GROUP, AclOperation.READ))
        pairs.append((ResourceType.GROUP, AclOperation.DESCRIBE))
        pairs.append((ResourceType.CLUSTER, AclOperation.IDEMPOTENT_WRITE))
        pairs.append((ResourceType.TRANSACTIONAL_ID, AclOperation.WRITE))
    if permission.role == 'admin':
        pairs.append((ResourceType.TOPIC, AclOperation.ALTER))
        pairs.append((ResourceType.TOPIC, AclOperation.ALTER_CONFIGS))
        pairs.append((ResourceType.TOPIC, AclOperation.DELETE))
        pairs.append((ResourceType.CLUSTER, AclOperation.CREATE))
        pairs.append((ResourceType.CLUSTER, AclOperation.DESCRIBE_CONFIGS))
    if permission.role == 'mdb_monitor':
        pairs.append((ResourceType.GROUP, AclOperation.READ))
        pairs.append((ResourceType.GROUP, AclOperation.DESCRIBE))
        pairs.append((ResourceType.TOPIC, AclOperation.DESCRIBE))

    principal = 'User:' + str(username)
    bindings = set()
    for pair in pairs:
        resource_type = pair[0]
        operation = pair[1]

        pattern_type = PatternType.LITERAL
        if resource_type == ResourceType.TOPIC:
            resource_name, pattern_type = pattern_and_pattern_type(context, permission.topic_name)
        elif resource_type == ResourceType.CLUSTER:
            resource_name = "kafka-cluster"
        elif resource_type == ResourceType.GROUP:
            resource_name, pattern_type = pattern_and_pattern_type(context, permission.group)
        elif resource_type == ResourceType.TRANSACTIONAL_ID:
            resource_name = "*"
        else:
            raise NotImplementedError("unsupported resource type {type}".format(type=resource_type.name()))

        resource_pattern = ResourcePattern(resource_type, resource_name, pattern_type)
        entry = AccessControlEntry(principal, permission.host, operation, AclPermissionType.ALLOW)
        binding = AclBinding(resource_pattern, entry)
        bindings.add(binding)

    return bindings


def build_acls_report(to_add, to_delete):
    report = {
        'add': sorted([binding_to_report_line(binding) for binding in to_add]),
        'remove': sorted([binding_to_report_line(binding) for binding in to_delete]),
    }
    return {k: v for k, v in report.items() if v}


def binding_to_report_line(binding):
    resource_type = binding.pattern().resourceType()
    resource_name = binding.pattern().name()
    pattern_type = binding.pattern().patternType()
    if pattern_type == 'PREFIXED':
        resource_name += '*'
    operation = binding.entry().operation()
    return "{} {} {}".format(resource_type.name(), resource_name, operation.name())


def binding_to_binding_filter(context, binding):
    pattern = binding.pattern()
    resource_pattern_filter = context.pkg.common.resource.ResourcePatternFilter(
        pattern.resourceType(), pattern.name(), pattern.patternType()
    )
    entry = binding.entry()
    entry_filter = context.pkg.common.acl.AccessControlEntryFilter(
        entry.principal(), entry.host(), entry.operation(), entry.permissionType()
    )
    return context.pkg.common.acl.AclBindingFilter(resource_pattern_filter, entry_filter)


def grant_monitor_role_py4j(username, test=True):
    permission_dicts = [{'topic_name': '*', 'role': 'mdb_monitor'}]
    return sync_acls(username, permission_dicts, test)
