# -*- coding: utf-8 -*-
"""
Apache Kafka management commands for MDB
"""
from collections import defaultdict
from functools import wraps
import logging
from operator import itemgetter
import random
import time
from traceback import format_exc

try:
    # Import salt module, but not in arcadia tests
    from salt.exceptions import CommandExecutionError, CommandNotFoundError
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}
__pillar__ = {}


log = logging.getLogger(__name__)
DEBUG_LEVEL = logging.ERROR
USE_PY4J_BY_DEFAULT = False


def __virtual__():
    return True


def process_error(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except (CommandNotFoundError, CommandExecutionError) as err:
            if args:
                name = args[0]
            else:
                name = func.__name__
            return {
                'name': name,
                'result': False,
                'comment': 'Error in apply: {0!r}: {1}'.format(name, err),
                'changes': {},
            }

    return wrapper


def _sync_topic_replication_factor(name, new_replication_factor):
    # Preparing brokers info
    nodes = __salt__['pillar.get']('data:kafka:nodes', {})
    if not nodes:
        raise CommandExecutionError('No brokers found in pillar')
    if len(nodes) < new_replication_factor:
        raise CommandExecutionError('Not enough brokers to increase replication factor')
    brokers = {}
    racks = set()
    for node in nodes.values():
        brokers[node['id']] = node
        brokers[node['id']]['replicas'] = 0
        racks.add(node['rack'])

    # Preparing replicas
    topic_replicas = __salt__['mdb_kafka.get_topic_replicas'](name)
    for partition, replicas in topic_replicas.items():
        for broker_id in replicas:
            brokers[broker_id]['replicas'] += 1

    new_partitions = {}
    changes = False
    for partition, replicas in topic_replicas.items():
        need_replicas = new_replication_factor - len(replicas)
        if need_replicas == 0:
            continue

        changes = True

        if need_replicas < 0:
            new_partitions[partition] = replicas[:new_replication_factor]
            continue

        new_partitions[partition] = list(replicas)

        # Looking for brokers not used in partition
        broker_ids = list(brokers.keys())
        broker_ids = set(broker_ids) - set(replicas)
        assert len(broker_ids) >= need_replicas
        # Simple case - number of replicas equal to number of brokers
        if len(broker_ids) == need_replicas:
            new_partitions[partition] = replicas + list(broker_ids)
            for broker_id in broker_ids:
                brokers[broker_id]['replicas'] += 1
            continue

        # Trying to put replicas in other availability zones
        preferred_zones = racks - {brokers[broker_id]['rack'] for broker_id in replicas}
        while preferred_zones and need_replicas:
            brokers_in_preferred_zones = [bid for bid in broker_ids if brokers[bid]['rack'] in preferred_zones]
            sorted_brokers = sorted([brokers[bid] for bid in brokers_in_preferred_zones], key=itemgetter('replicas'))
            if not sorted_brokers:
                break
            broker = sorted_brokers[0]
            new_partitions[partition].append(broker['id'])
            broker['replicas'] += 1
            need_replicas -= 1
            preferred_zones -= {broker['rack']}

        # Choosing brokers that holds minimum replicas
        available_brokers = [brokers[bid] for bid in broker_ids if bid not in new_partitions[partition]]
        if len(available_brokers) < need_replicas:
            raise CommandExecutionError('Not enough brokers to increase replication factor')
        sorted_brokers = sorted(available_brokers, key=itemgetter('replicas'))
        for i in range(need_replicas):
            broker = sorted_brokers[i]
            new_partitions[partition].append(broker['id'])
            broker['replicas'] += 1

    if changes and not __opts__.get('test'):
        __salt__['mdb_kafka.reassign_partitions'](name, new_partitions)

    return changes


def _sync_topic_config(name, config):
    known_topic_config_properties = __salt__['pillar.get']('data:kafka:known_topic_config_properties', [])
    changes, target_config = __salt__['mdb_kafka.topic_config_planned_changes'](
        name, config, known_topic_config_properties
    )
    if not changes:
        return
    if not __opts__.get('test'):
        __salt__['mdb_kafka.alter_topic_config'](name, dict(target_config))
    return changes


@process_error
def topic_exists(name, partitions, replication_factor=3, config=None):
    """
    Creates topic with passed params if not exists
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret

    if not isinstance(partitions, int):
        raise CommandExecutionError("Partitions must be int " + str(partitions))
    if not isinstance(replication_factor, int):
        raise CommandExecutionError("Replication factor must be digit-only " + str(replication_factor))

    config = config or {}
    if 'min.insync.replicas' in config:
        pass
    elif replication_factor < 3:
        config['min.insync.replicas'] = '1'
    else:
        config['min.insync.replicas'] = '2'

    # Create topic if not exists
    topic = __salt__['mdb_kafka.get_topic'](name)
    if not topic:
        if not __opts__.get('test'):
            __salt__['mdb_kafka.create_topic'](name, partitions, replication_factor, config)
        ret['changes'] = {name: 'created'}
        return ret

    if len(topic.partitions) > partitions:
        msg = 'Trying to decrease partitions on topic "{}" from {} to {}'
        msg = msg.format(name, len(topic.partitions), partitions)
        raise CommandExecutionError(msg)

    # Partitions changes
    if partitions > len(topic.partitions):
        if not __opts__.get('test'):
            __salt__['mdb_kafka.increase_topic_partitions'](name, partitions)
        ret['changes'][name + '_partitions'] = 'increased to ' + str(partitions)

    # Replication factor changes
    replication_factor_changed = _sync_topic_replication_factor(name, replication_factor)
    if replication_factor_changed:
        ret['changes'][name + '_replication_factor'] = 'changed to ' + str(replication_factor)

    # Config changes
    config_changes = _sync_topic_config(name, config)
    if config_changes:
        ret['changes'][name + '_config'] = config_changes

    if not ret['changes']:
        ret['comment'] = "Topic '{name}' already exists and configured".format(name=name)
    return ret


@process_error
def topic_absent(name):
    """
    Deletes topic if it exists
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret

    existing_topics = __salt__['mdb_kafka.list_topics']()
    if name not in existing_topics:
        ret['comment'] = "Topic '{name}' already absent".format(name=name)
        return ret

    if __opts__.get('test'):
        ret['changes'] = {name: 'deleted'}
        return ret

    deleted = __salt__['mdb_kafka.delete_topic'](name)
    if deleted:
        ret['changes'] = {name: 'deleted'}
    else:
        ret['comment'] = "Topic '{name}' already absent".format(name=name)
    return ret


@process_error
def topics_sync(name):
    """
    Sync kafka topics with pillar data
    """
    log.log(DEBUG_LEVEL, '[topics_sync] started at {time}'.format(time=time.time()))

    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret

    if __salt__['pillar.get']('data:kafka:unmanaged_topics') and not __salt__['pillar.get']('data:kafka:sync_topics'):
        ret['comment'] = 'Skipping topics sync: unmanaged mode'
        return ret

    if __salt__['pillar.get']('data:kafka:config:auto.create.topics.enable'):
        ret['comment'] = 'Skipping topics sync: auto create enabled'
        return ret

    log.log(DEBUG_LEVEL, '[topics_sync] finished checking pillar items at {time}'.format(time=time.time()))

    now = time.time()
    existing_topics = __salt__['mdb_kafka.list_topics']()
    log.log(DEBUG_LEVEL, '[topics_sync] time to list kafka topics {delta}'.format(delta=time.time() - now))

    now = time.time()
    expected_topics = __salt__['pillar.get']('data:kafka:topics')
    log.log(DEBUG_LEVEL, '[topics_sync] time to list pillar topics {delta}'.format(delta=time.time() - now))

    deleted_topics = __salt__['pillar.get']('data:kafka:deleted_topics', {})
    for name, topic in expected_topics.items():
        now = time.time()
        topic_ret = topic_exists(**topic)
        log.log(
            DEBUG_LEVEL,
            '[topics_sync] time to create topic {name} is {delta}'.format(name=topic['name'], delta=time.time() - now),
        )
        if not _merge_state_apply_results(ret, topic_ret):
            return ret
    for name in existing_topics:
        if name.startswith("__") or name in expected_topics:
            continue
        if name not in deleted_topics:
            continue
        topic_ret = topic_absent(name)
        if not _merge_state_apply_results(ret, topic_ret):
            return ret
    log.log(DEBUG_LEVEL, '[topics_sync] finished at {time}'.format(time=time.time()))
    return ret


@process_error
def user_auth_exisis(name, password, force_zk=False):
    return user_auth_exists(name, password, force_zk)


@process_error
def user_auth_exists(name, password, force_zk=False):
    """
    Creates user if not exists
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret

    use_zk = True
    list_users = __salt__['mdb_kafka.list_users']
    create_user_auth = __salt__['mdb_kafka.create_user_auth']
    if __salt__['pillar.get']('data:kafka:acls_via_py4j', USE_PY4J_BY_DEFAULT) and not force_zk:
        use_zk = False
        list_users = __salt__['mdb_kafka.list_users_py4j']
        create_user_auth = __salt__['mdb_kafka.create_user_auth_py4j']

    # optimization: state mdb-kafka-admin-user does something useful only during the first call
    # but subsequent invocations also make rather expensive calls to mdb_kafka.list_users
    if use_zk and name == 'mdb_admin' and __salt__['mdb_kafka.user_password_matches'](name, password):
        ret['comment'] = "User '{name}' already exists".format(name=name)
        return ret

    existing_users = list_users()
    if name not in existing_users:
        if not __opts__.get('test'):
            create_user_auth(name, password)
        ret['changes'] = {name: {'created': True}}
        return ret

    if __salt__['mdb_kafka.user_password_matches'](name, password):
        ret['comment'] = "User '{name}' already exists".format(name=name)
        return ret

    if not __opts__.get('test'):
        create_user_auth(name, password)
    ret['changes'] = {name: {'password changed': True}}
    return ret


@process_error
def monitor_user_exists(name):
    """
    Create user with role if not exists
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    username = 'mdb_monitor'
    password = __salt__['mdb_kafka.monitor_password']()
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret

    result = user_auth_exists(username, password)
    if not _merge_state_apply_results(ret, result):
        return ret

    if __salt__['pillar.get']('data:kafka:acls_via_py4j', USE_PY4J_BY_DEFAULT):
        ret['changes'] = result['changes']
        changes = __salt__['mdb_kafka.grant_monitor_role_py4j'](username, __opts__.get('test'))
        if changes:
            ret['changes'].setdefault(username, {}).update(changes)
    else:
        if not result['changes']:
            return ret

        if not __opts__.get('test'):
            __salt__['mdb_kafka.grant_monitor_role'](username)

        ret['changes'] = {username: 'user created'}
    return ret


@process_error
def user_auth_absent(name):
    """
    Delete user if exists
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret

    list_users = __salt__['mdb_kafka.list_users']
    clean_user_roles = __salt__['mdb_kafka.clean_user_roles']
    delete_user_auth = __salt__['mdb_kafka.delete_user_auth']
    if __salt__['pillar.get']('data:kafka:acls_via_py4j', USE_PY4J_BY_DEFAULT):
        list_users = __salt__['mdb_kafka.list_users_py4j']
        clean_user_roles = __salt__['mdb_kafka.clean_user_roles_py4j']
        delete_user_auth = __salt__['mdb_kafka.delete_user_auth_py4j']

    existing_users = list_users()
    if name not in existing_users:
        ret['comment'] = "User '{name}' already absent".format(name=name)
        return ret

    if not __opts__.get('test'):
        clean_user_roles(name)
        delete_user_auth(name)
    ret['changes'] = {name: 'deleted'}
    return ret


def _has_consumer_role(user):
    for perm in user['permissions']:
        role = perm['role']
        if role == 'consumer' or role == 'admin':
            return True
    return False


def _sync_user_roles(user):
    changes = {}
    username = user['name']

    topics_roles_expected = {}
    for perm in user['permissions']:
        roles = topics_roles_expected.get(perm['topic_name'], set())
        roles.add(perm['role'])
        topics_roles_expected[perm['topic_name']] = roles
    for topic_name, roles in topics_roles_expected.items():
        if 'admin' in roles:
            # https://st.yandex-team.ru/MDB-15060
            topics_roles_expected[topic_name] = roles - {'producer', 'consumer'}

    topics_denied = set()
    topics_roles_existing = __salt__['mdb_kafka.user_topic_roles'](username)
    for topic, roles in topics_roles_existing.items():
        expected_roles = topics_roles_expected.get(topic, [])
        for role in roles:
            if role not in expected_roles:
                if not __opts__.get('test'):
                    __salt__['mdb_kafka.deny_role'](username, topic, role)
                changes.update({username + '_' + role: 'denied'})
                topics_denied.add(topic)

    for topic, roles in topics_roles_expected.items():
        existing_roles = topics_roles_existing.get(topic, [])
        # if we deleted roles from topic, we need to grand permissions again
        if topic in topics_denied:
            existing_roles = []
        for role in roles:
            if role not in existing_roles:
                if not __opts__.get('test'):
                    __salt__['mdb_kafka.grant_role'](username, topic, role)
                changes.update({username + '_' + role: 'granted'})

    if _has_consumer_role(user):
        __salt__['mdb_kafka.grant_common_consumer_permissions'](username)

    return changes


@process_error
def ensure_user_role(name, topic, role):
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    if __salt__['pillar.get']('data:kafka:acls_via_py4j', USE_PY4J_BY_DEFAULT):
        permissions = [{'topic_name': topic, 'role': role}]
        ret['changes'] = __salt__['mdb_kafka.add_permissions'](name, permissions, __opts__.get('test'))
    else:
        topics_roles_existing = __salt__['mdb_kafka.user_topic_roles'](name)
        existing_roles = topics_roles_existing.get(topic, [])
        if role in existing_roles:
            ret['comment'] = "User '{name}' already have role '{role}' for topic '{topic}'".format(
                name=name, topic=topic, role=role
            )
            return ret
        if not __opts__.get('test'):
            __salt__['mdb_kafka.grant_role'](name, topic, role)
        ret['changes'].update({name + '_' + role: 'granted'})
        if role == 'consumer':
            __salt__['mdb_kafka.grant_common_consumer_permissions'](name)
    return ret


def _merge_state_apply_results(parent_result, child_result):
    parent_result['changes'].update(child_result['changes'])
    if not child_result['result']:
        parent_result['result'] = False
        parent_result['comment'] = child_result['comment']
    return child_result['result']


@process_error
def users_sync(name, username=None):
    """
    Sync kafka topics with pillar data
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret

    acls_via_py4j = __salt__['pillar.get']('data:kafka:acls_via_py4j', USE_PY4J_BY_DEFAULT)
    list_users = __salt__['mdb_kafka.list_users']
    if acls_via_py4j:
        list_users = __salt__['mdb_kafka.list_users_py4j']

    now = time.time()
    existing_users = list_users()
    log.log(DEBUG_LEVEL, '[users_sync] time mdb_kafka.list_users: {delta}'.format(delta=time.time() - now))

    expected_users = __salt__['pillar.get']('data:kafka:users')
    deleted_users = __salt__['pillar.get']('data:kafka:deleted_users', {})

    # remove users that not in expected
    for name in existing_users:
        if username and name != username:
            continue
        if name.startswith("mdb_") or name in expected_users:
            continue
        if name not in deleted_users:
            continue
        result = user_auth_absent(name)
        if not _merge_state_apply_results(ret, result):
            return ret

    for name, user in expected_users.items():
        if username and name != username:
            continue
        if name.startswith("mdb_"):
            continue
        now = time.time()
        result = user_auth_exists(name, user['password'])
        log.log(
            DEBUG_LEVEL, '[users_sync] time to create user {user}: {delta}'.format(user=name, delta=time.time() - now)
        )

        if not _merge_state_apply_results(ret, result):
            return ret
        user_created = result['changes'].get(name, {}).get('created')

        now = time.time()
        if acls_via_py4j:
            changes = __salt__['mdb_kafka.sync_acls'](user['name'], user['permissions'], __opts__.get('test'))
            if changes:
                ret['changes'].setdefault(name, {}).update(changes)
        else:
            sync_roles_changes = _sync_user_roles(user)
            if not user_created:
                ret['changes'].update(sync_roles_changes)
        log.log(
            DEBUG_LEVEL, '[users_sync] time to sync ACLs for {user}: {delta}'.format(user=name, delta=time.time() - now)
        )

    return ret


def wait_for_zk(name, wait_timeout=600):
    """
    Wait for zookeeper to start up.
    """
    ret = {'name': name, 'result': False, 'changes': {}, 'comment': ''}

    if __opts__['test']:
        ret['result'] = True
        return ret

    try:
        __salt__['mdb_kafka.wait_for_zk'](wait_timeout=wait_timeout)
    except Exception:
        ret['result'] = False
        ret['comment'] += format_exc()
        return ret

    ret['result'] = True
    return ret


class ReplicasState:
    def __init__(self, nodes, topic_replicas):
        self.racks = {}
        rack_ids = {node['rack'] for node in nodes.values()}
        for rack_id in rack_ids:
            self.racks[rack_id] = {
                'id': rack_id,
                'brokers': set(),
                'partitions': set(),
            }

        self.brokers = {}
        for node in nodes.values():
            self.brokers[node['id']] = node
            self.brokers[node['id']]['partitions'] = set()
            self.racks[node['rack']]['brokers'].add(node['id'])

        self.replication_factor = 0
        self.topic_replicas = defaultdict(list)
        self.removed_topic_replicas = defaultdict(list)
        for partition, replicas in topic_replicas.items():
            self.replication_factor = max(self.replication_factor, len(replicas))
            for broker_id in replicas:
                if broker_id in self.brokers:
                    self.brokers[broker_id]['partitions'].add(partition)
                    self.racks[self.brokers[broker_id]['rack']]['partitions'].add(partition)
                    self.topic_replicas[partition].append(broker_id)
                else:
                    # Host is removed from pillar, will rebalance partitions from it
                    self.removed_topic_replicas[partition].append(broker_id)

    def racks_sorted(self):
        '''
        Return racks sorted by number of served partitions
        '''
        racks_sorted = sorted([(len(rack['partitions']), rack_id) for rack_id, rack in self.racks.items()])
        return [(qty, self.racks[rack_id]) for qty, rack_id in racks_sorted]

    def brokers_sorted(self, rack_id=None):
        '''
        Return list of brokers sorted by quantity of holed partitions
        Return type: [(quantity: int, broker: dict)]
        '''
        brokers = []
        for broker in self.brokers.values():
            if rack_id and rack_id != broker['rack']:
                continue
            brokers.append((len(broker['partitions']), broker['id']))
        brokers_sorted = sorted(brokers)
        return [(qty, self.brokers[broker_id]) for qty, broker_id in brokers_sorted]

    def rebuild_rack_partitions(self, rack_id):
        rack = self.racks[rack_id]
        brokers_partitions = [self.brokers[broker_id]['partitions'] for broker_id in rack['brokers']]
        rack_partitions = set()
        rack_partitions.update(*brokers_partitions)
        rack['partitions'] = rack_partitions

    def move_partition(self, partition, broker_id_from, broker_id_to, strict=False):
        replicas = self.topic_replicas[partition]
        if broker_id_from in replicas:
            replicas.remove(broker_id_from)
        if broker_id_from in self.brokers:
            broker_from = self.brokers[broker_id_from]
            broker_from['partitions'].remove(partition)
            self.rebuild_rack_partitions(broker_from['rack'])

        if broker_id_to not in replicas:
            replicas.append(broker_id_to)
        else:
            if strict:
                raise RuntimeError(
                    'Attempt to add broker {broker_id_to} to replicas twice for partition {partition}'.format(
                        **locals()
                    )
                )
        broker_to = self.brokers[broker_id_to]
        broker_to['partitions'].add(partition)
        self.racks[broker_to['rack']]['partitions'].add(partition)

    def fix_replication_racks(self):
        '''
        Ensure that two replicas of one partition not placed in one rack
        '''
        for rack in self.racks.values():
            brokers = list(rack['brokers'])
            i = 0
            # Looking for same partitions on differents brokers in one rack
            for i in range(len(brokers)):
                broker_out = self.brokers[brokers[i]]
                for j in range(i + 1, len(brokers)):
                    partitions_to_move = broker_out['partitions'] & self.brokers[brokers[j]]['partitions']
                    for partition_to_move in partitions_to_move:
                        racks_used = {
                            self.brokers[broker_id]['rack'] for broker_id in self.topic_replicas[partition_to_move]
                        }
                        rack_to_id = (set(self.racks.keys()) - racks_used).pop()
                        broker_to_id = set(self.racks[rack_to_id]['brokers']).pop()
                        self.move_partition(partition_to_move, broker_out['id'], broker_to_id)

    def ensure_replication_racks(self):
        '''
        Ensure that every partition has replica in each rack (replication factor bigger than number of racks)
        '''
        for partition, replicas in self.topic_replicas.items():
            partition_racks = {self.brokers[broker_id]['rack'] for broker_id in replicas}
            missing_racks = set(self.racks.keys()) - partition_racks
            for missing_rack_id in missing_racks:
                _, broker_to = self.brokers_sorted(missing_rack_id)[0]
                # Sorting racks by number of replicas of this partition
                racks_with_replicas = [
                    (set(replicas) & self.racks[rack_id]['brokers'], rack_id) for rack_id in partition_racks
                ]
                broker_ids_from, rack_id_from = sorted(racks_with_replicas, key=lambda x: len(x[0])).pop()
                broker_id_from = broker_ids_from.pop()
                self.move_partition(partition, broker_id_from, broker_to['id'])

    def balance_replicas_among_racks(self):
        '''
        Ensure uniform distribution replicas among racks
        '''
        if len(self.racks.items()) < 2:
            return
        while True:
            racks_sorted = self.racks_sorted()
            max_replicas, max_rack = racks_sorted.pop()
            min_replicas, min_rack = racks_sorted[0]
            if (max_replicas - min_replicas) < 2:
                break

            # Choosing partition from max rack that absent on a min rack
            partition_to_move = (max_rack['partitions'] - min_rack['partitions']).pop()

            # Choosing broker that holds partition to move
            broker_id_from = (set(self.topic_replicas[partition_to_move]) & max_rack['brokers']).pop()
            # Chhosing broker in a min rack with minimum partitions to minimize move on a next steps
            _, broker_to = self.brokers_sorted(min_rack['id'])[0]
            self.move_partition(partition_to_move, broker_id_from, broker_to['id'])

    def balance_replicas_among_brokers(self):
        '''
        Ensure uniform distribution replicas among brokers in one rack
        '''
        for rack_id, rack in self.racks.items():
            if len(rack['brokers']) < 2:
                continue
            while True:
                brokers_sorted = self.brokers_sorted(rack_id)
                max_replicas, max_broker = brokers_sorted.pop()
                min_replicas, min_broker = brokers_sorted[0]
                if (max_replicas - min_replicas) < 2:
                    break
                # Choosing any partition from max broker
                partition_to_move = (max_broker['partitions'] - min_broker['partitions']).pop()
                self.move_partition(partition_to_move, max_broker['id'], min_broker['id'])

    def balance_replicas_among_all_brokers(self):
        '''
        Ensure uniform distribution replicas among all brokers
        '''
        while True:
            brokers_sorted = self.brokers_sorted()
            max_replicas, max_broker = brokers_sorted.pop()
            min_replicas, min_broker = brokers_sorted[0]
            if (max_replicas - min_replicas) < 2:
                break
            # Choosing any partition from max broker
            partition_to_move = (max_broker['partitions'] - min_broker['partitions']).pop()
            self.move_partition(partition_to_move, max_broker['id'], min_broker['id'])

    def get_leaders_info(self):
        leaders_brokers = {}
        for broker_id in self.brokers:
            leaders_brokers[broker_id] = set()
        leaders_racks = {}
        for rack_id in self.racks:
            leaders_racks[rack_id] = set()

        for partition, replicas in self.topic_replicas.items():
            broker_id = replicas[0]
            leaders_brokers[broker_id].add(partition)
            rack_id = self.brokers[broker_id]['rack']
            leaders_racks[rack_id].add(partition)
        return leaders_racks, leaders_brokers

    def set_leader(self, partition, broker_id):
        '''
        Changing order of partitions to change leader
        '''
        replicas = self.topic_replicas[partition]
        if broker_id not in replicas:
            raise ValueError('Broker {} not found in {} replicas: {}'.format(broker_id, partition, replicas))
        replicas.remove(broker_id)
        replicas.insert(0, broker_id)

    def set_replica(self, partition, broker_id):
        '''
        Changing order of partitions to change leader
        '''
        replicas = self.topic_replicas[partition]
        if broker_id not in replicas:
            raise ValueError('Broker {} not found in {} replicas: {}'.format(broker_id, partition, replicas))
        replicas.remove(broker_id)
        replicas.append(broker_id)

    def balance_leaders_among_racks(self):
        '''
        Ensure uniform distribution leaders among racks
        '''
        if len(self.racks) < 2:
            return
        while True:
            leaders_racks, leaders_brokers = self.get_leaders_info()
            racks_sorted = sorted([(len(leaders_rack), rack_id) for rack_id, leaders_rack in leaders_racks.items()])
            max_leaders, max_rack_id = racks_sorted.pop()
            min_leaders, min_rack_id = racks_sorted[0]
            if (max_leaders - min_leaders) < 2:
                break

            # Looking for partition, that presents on max and min rack at the same time
            partition_to_move = (leaders_racks[max_rack_id] & self.racks[min_rack_id]['partitions']).pop()
            # Changing order of partitions to change leader
            replicas = self.topic_replicas[partition_to_move]
            new_leader_id = (set(replicas) & self.racks[min_rack_id]['brokers']).pop()
            self.set_leader(partition_to_move, new_leader_id)

    def balance_leaders_among_brokers(self):
        '''
        Ensure uniform distribution leaders among brokers in one rack
        '''
        for rack_id, rack in self.racks.items():
            if len(rack['brokers']) < 2:
                continue
            while True:
                leaders_racks, leaders_brokers = self.get_leaders_info()
                brokers_sorted = sorted([(len(leaders_brokers[broker_id]), broker_id) for broker_id in rack['brokers']])
                max_leader, max_broker_id = brokers_sorted.pop()
                min_leader, min_broker_id = brokers_sorted[0]
                if (max_leader - min_leader) < 2:
                    break
                # Any leader partition on max broker
                partition_to_move_out = leaders_brokers[max_broker_id].pop()
                # Any replica partition on min broker
                partition_to_move_in = (
                    self.brokers[min_broker_id]['partitions'] - leaders_brokers[min_broker_id]
                ).pop()
                # Exchanging leader partition with replica partition
                self.move_partition(partition_to_move_out, max_broker_id, min_broker_id)
                self.set_leader(partition_to_move_out, min_broker_id)
                self.move_partition(partition_to_move_in, min_broker_id, max_broker_id)

    def balance_leaders_among_all_brokers(self):
        '''
        Ensure uniform distribution leaders among all brokers
        '''
        # Preventing infinite loop
        counter = 10000
        while counter:
            counter -= 1
            leaders_racks, leaders_brokers = self.get_leaders_info()
            brokers_sorted = sorted([(len(leaders), broker_id) for broker_id, leaders in leaders_brokers.items()])
            max_leader, max_broker_id = brokers_sorted.pop()
            min_leader, min_broker_id = brokers_sorted[0]
            if (max_leader - min_leader) < 2:
                break
            # Trying to move from max to min
            partitions_to_move_out = leaders_brokers[max_broker_id] & self.brokers[min_broker_id]['partitions']
            if not partitions_to_move_out:
                # Or just a random partition from max
                partitions_to_move_out = leaders_brokers[max_broker_id]
            partition_to_move_out = random.choice(list(partitions_to_move_out))
            self.set_replica(partition_to_move_out, max_broker_id)

    def replace_removed_replicas(self):
        for partition, removed_brokers in self.removed_topic_replicas.items():
            for removed_broker_id in removed_brokers:

                min_broker = next(
                    (
                        broker
                        for _, broker in self.brokers_sorted()
                        if broker['id'] not in self.topic_replicas[partition]
                    ),
                    None,
                )
                if min_broker is None:
                    raise RuntimeError(
                        'No free broker for partition {partition}, current brokers: '
                        '{self.topic_replicas[partition]}'.format(**locals())
                    )
                self.move_partition(partition, removed_broker_id, min_broker['id'])

    @property
    def balance_racks(self):
        return len(self.racks) >= self.replication_factor

    def balance(self):
        self.replace_removed_replicas()
        if self.balance_racks:
            self.fix_replication_racks()
            self.balance_replicas_among_racks()
            self.balance_replicas_among_brokers()
            self.balance_leaders_among_racks()
            self.balance_leaders_among_brokers()
        else:
            self.ensure_replication_racks()
            self.balance_replicas_among_all_brokers()
            self.balance_leaders_among_all_brokers()


def topic_rebalance(name):
    """
    Rebalance kafka topics
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret

    for name in __salt__['mdb_kafka.list_topics']():
        nodes = __salt__['pillar.get']('data:kafka:nodes', {})
        topic_replicas = __salt__['mdb_kafka.get_topic_replicas'](name)
        state = ReplicasState(nodes, topic_replicas)
        state.balance()
        new_partitions = state.topic_replicas
        changes = False
        for partition, replicas in __salt__['mdb_kafka.get_topic_replicas'](name).items():
            if replicas != new_partitions[partition]:
                changes = True
                break

        if changes:
            ret['changes'][name] = __salt__['mdb_kafka.reassignment_file_content'](name, new_partitions)
            if not __opts__.get('test'):
                __salt__['mdb_kafka.reassign_partitions'](name, new_partitions)

    return ret
