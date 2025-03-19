# -*- coding: utf-8 -*-
"""
Apache Kafka Connect management commands for MDB
"""

from functools import wraps
import logging

# from .mdb_kafka import process_error, _merge_state_apply_results

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


def __virtual__():
    return True


def process_error(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        if args:
            name = args[0]
        else:
            name = func.__name__
        try:
            return func(*args, **kwargs)
        except (CommandNotFoundError, CommandExecutionError) as err:
            return {
                'name': name,
                'result': False,
                'comment': 'Error in apply: {0!r}: {1}'.format(name, err),
                'changes': {},
            }
        except ValueError as err:
            return {
                'name': name,
                'result': False,
                'comment': 'Validation error: {}'.format(err),
                'changes': {},
                'retcode': 3,
            }

    return wrapper


def _merge_state_apply_results(parent_result, child_result):
    parent_result['changes'].update(child_result['changes'])
    if not child_result['result']:
        parent_result['result'] = False
        parent_result['comment'] = child_result['comment']
    return child_result['result']


@process_error
def connector_absent(name):
    """
    Delete connector if exists
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret

    if not __salt__['mdb_kafka_connect.connector_exists'](name):
        ret['comment'] = "Connector '{name}' not exists".format(name=name)
        return ret

    if not __opts__.get('test'):
        __salt__['mdb_kafka_connect.delete_connector'](name)
    ret['changes'] = {name: 'deleted'}
    return ret


def validate_connector_config(config):
    class_name = config['connector.class']
    __salt__['mdb_kafka_connect.validate_connector_config'](class_name, config)


@process_error
def connector_exist(name, connector_data):
    """
    Create connector if not exists,
    otherwise update it's config.
    Function receive plain config
    of the connector.
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    plain_config = __salt__['mdb_kafka_connect.make_plain_connector_config'](connector_data)
    validate_connector_config(plain_config)
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret
    existing_connectors = __salt__['mdb_kafka_connect.list_connectors']()
    if name not in existing_connectors:
        if not __opts__.get('test'):
            __salt__['mdb_kafka_connect.create_connector'](name, plain_config)
        ret['changes'] = {name: 'created'}
        return ret
    if __salt__['mdb_kafka_connect.connector_config_matches'](name, plain_config):
        ret['comment'] = "Connector '{name}' already exists".format(name=name)
        return ret
    if not __opts__.get('test'):
        __salt__['mdb_kafka_connect.update_connector_config'](name, plain_config)
    ret['changes'] = {name: 'config changed'}
    return ret


@process_error
def sync_connectors(name, connector_name):
    """
    Sync kafka connectors with pillar data
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret

    existing_connectors_names = __salt__['mdb_kafka_connect.list_connectors']()
    expected_connectors = __salt__['pillar.get']('data:kafka:connectors')
    deleted_connectors = __salt__['pillar.get']('data:kafka:deleted_connectors', {})

    # Remove connectors that not in expected
    for name in existing_connectors_names:
        if connector_name and connector_name != name:
            continue
        if name in expected_connectors:
            continue
        if name not in deleted_connectors:
            continue
        result = connector_absent(name)
        if not _merge_state_apply_results(ret, result):
            return ret

    # Add or update connectors
    # that is expected
    for name, connector_data in expected_connectors.items():
        if connector_name and name != connector_name:
            continue
        result = connector_exist(name, connector_data)
        if not _merge_state_apply_results(ret, result):
            return ret
    return ret


@process_error
def connector_paused(name):
    """
    Pause connector if it exists
    and running or unassigned
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret
    if not __salt__['mdb_kafka_connect.connector_exists'](name):
        ret['comment'] = "Connector '{name}' not exists".format(name=name)
        return ret
    status = __salt__['mdb_kafka_connect.get_connector_status'](name)
    if status == 'FAILED':
        raise CommandExecutionError('Connector \"' + name + '\" failed')
    if status == 'PAUSED':
        ret['comment'] = "Connector '{name}' already paused".format(name=name)
        return ret
    if not __opts__.get('test'):
        __salt__['mdb_kafka_connect.connector_pause'](name)
    ret['changes'] = {name: 'paused'}
    return ret


@process_error
def connector_running(name):
    """
    Run connector if it exists
    and paused or unassigned
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if not __salt__['mdb_kafka.making_meta_changes']():
        ret['comment'] = 'Skipping on this broker'
        return ret
    if not __salt__['mdb_kafka_connect.connector_exists'](name):
        ret['comment'] = "Connector '{name}' not exists".format(name=name)
        return ret
    status = __salt__['mdb_kafka_connect.get_connector_status'](name)
    if status == 'FAILED':
        raise CommandExecutionError('Connector \"' + name + '\" failed')
    if status == 'RUNNING':
        ret['comment'] = "Connector '{name}' already running".format(name=name)
        return ret
    if not __opts__.get('test'):
        __salt__['mdb_kafka_connect.connector_resume'](name)
    ret['changes'] = {name: 'resumed'}
    return ret
