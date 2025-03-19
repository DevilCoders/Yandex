"""
Simple zookeeper mock
"""

import json

from kazoo.exceptions import NoNodeError

from .utils import handle_action


def _react(state, operation, path, data):
    """
    Run hooks emulating external services reactions to zk changes
    """
    for reaction in state['zookeeper']['reactions']:
        reaction(state['zookeeper'], operation, path, data)


def _get_dict(root_dict, path):
    """
    Get subdict in root by unix-like path
    """
    splitted = path.split('/')
    current = root_dict
    for element in splitted:
        if element:
            if not isinstance(current, dict) or not current:
                return False
            current = current.get(element, False)
    return current


def _rec_create(root_dict, path, data):
    """
    Create path in dict if not exists
    """
    splitted = path.split('/')
    current = root_dict
    for element in splitted[:-1]:
        if element:
            if not current.get(element):
                current[element] = {}
            current = current[element]
    if not current.get(splitted[-1]):
        current[splitted[-1]] = data


def _rec_set(root_dict, path, data):
    """
    Update existing path in dict
    """
    splitted = path.split('/')
    res = _get_dict(root_dict, '/'.join(splitted[:-1]))
    if res is False or splitted[-1] not in res:
        raise NoNodeError(path)
    res[splitted[-1]] = data


def _rec_delete(root_dict, path):
    """
    Delete existing path in dict
    """
    splitted = path.split('/')
    res = _get_dict(root_dict, '/'.join(splitted[:-1]))
    if res is False or splitted[-1] not in res:
        return
    del res[splitted[-1]]


def get_children(state, path):
    """
    Get children mock
    """
    action_id = f'zookeeper-get_children-{path}'
    handle_action(state, action_id)
    res = _get_dict(state['zookeeper'], path)
    if res is False:
        raise NoNodeError(path)
    return res.keys()


def zk_create(state, path, data):
    """
    Create mock
    """
    action_id = f'zookeeper-create-{path}'
    handle_action(state, action_id)
    _rec_create(state['zookeeper'], path, data)
    _react(state, 'create', path, data)


def exists(state, path):
    """
    Exists mock
    """
    action_id = f'zookeeper-exists-{path}'
    handle_action(state, action_id)
    return bool(_get_dict(state['zookeeper'], path))


def delete(state, path):
    """
    Delete mock
    """
    action_id = f'zookeeper-delete-{path}'
    handle_action(state, action_id)
    _rec_delete(state, path)
    _react(state, 'delete', path, None)


def contenders(state):
    """
    Lock contenders mock
    """
    action_id = 'zookeeper-get-lock-contenders'
    handle_action(state, action_id)
    return state['zookeeper'].get('contenders', [])


def get(state, path):
    """
    Get mock
    """
    action_id = f'zookeeper-get-{path}'
    handle_action(state, action_id)
    res = _get_dict(state['zookeeper'], path)
    if res is False:
        raise NoNodeError(path)
    if isinstance(res, dict):
        return json.dumps(res).encode('utf-8'), None
    if isinstance(res, str):
        return res.encode('utf-8'), None
    return res, None


def zk_set(state, path, data):
    """
    Set mock
    """
    action_id = f'zookeeper-set-{path}'
    handle_action(state, action_id)
    _rec_set(state['zookeeper'], path, data)
    _react(state, 'set', path, data)


def reconfig(state, joining, leaving, new_members):
    """
    Reconfig mock
    """
    action_id = f'zookeeper-reconfig-{joining}-{leaving}-{new_members}'
    handle_action(state, action_id)


def ensure_path(state, path):
    """
    Ensure path mock
    """
    action_id = f'zookeeper-ensure_path-{path}'
    handle_action(state, action_id)
    _rec_create(state['zookeeper'], path, {})


def zookeeper(mocker, state):
    """
    Setup zookeeper mock
    """
    client = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.zookeeper.KazooClient')
    client.return_value.get_children.side_effect = lambda path: get_children(state, path)
    client.return_value.create.side_effect = lambda path, data: zk_create(state, path, data)
    client.return_value.delete.side_effect = lambda path, **_: delete(state, path)
    client.return_value.exists.side_effect = lambda path: exists(state, path)
    client.return_value.reconfig.side_effect = lambda joining, leaving, new_members: reconfig(
        state, joining, leaving, new_members
    )
    client.return_value.ensure_path.side_effect = lambda path: ensure_path(state, path)
    client.return_value.set.side_effect = lambda path, data: zk_set(state, path, data)
    client.return_value.get.side_effect = lambda path: get(state, path)
    client.return_value.Lock.return_value.contenders.side_effect = lambda: contenders(state)
