#!/usr/bin/env python
# -*- coding: utf-8 -*-

try:
    # Import salt module, but not in arcadia tests
    from salt.exceptions import CommandExecutionError, CommandNotFoundError
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}


__virtualname__ = 'porto_volume'


def __virtual__():
    if 'porto.volumes_list' in __salt__:
        return __virtualname__
    else:
        return False


def present(
    name,
    backend='native',
    user='root',
    group='root',
    permissions='0775',
    space_guarantee=None,
    space_limit=None,
    inode_guarantee=None,
    inode_limit=None,
    read_only=False,
):
    ret = {'name': name, 'result': None, 'comment': '', 'changes': {}}

    properties = {'backend': backend, 'user': user, 'group': group, 'permissions': permissions}
    if space_guarantee:
        properties['space_guarantee'] = space_guarantee
    if space_limit:
        properties['space_limit'] = space_limit
    if inode_guarantee:
        properties['inode_guarantee'] = inode_guarantee
    if inode_limit:
        properties['inode_limit'] = inode_limit

    try:
        volume = __salt__['porto.storage_info'](storage=name)
        if volume:
            path = volume.keys()[0]
            current = volume[path]
            need_update = {}

            for key, value in current.items():
                if key in properties and value != properties[key]:
                    need_update[key] = properties[key]

            if need_update:
                if __opts__['test']:
                    ret['result'] = None
                    ret['comment'] = 'Volume {0} will be changed.'.format(name)
                else:
                    volume = __salt__['porto.tune_volume'](path=path, **need_update)
                    ret['result'] = True
                    ret['comment'] = 'Volume {0} has been changed.'.format(name)

                ret['changes'] = need_update
            else:
                ret['result'] = True
                ret['comment'] = 'Volume {0} is in actual state.'.format(name)
        else:
            if __opts__['test']:
                ret['result'] = None
                ret['comment'] = 'Volume {0} will be created.'.format(name)
            else:
                volume = __salt__['porto.create_volume'](storage=name, **properties)
                ret['result'] = True
                ret['comment'] = 'Volume {0} has been created.'.format(name)

            ret['changes'] = properties

        return ret
    except (CommandNotFoundError, CommandExecutionError) as err:
        ret['result'] = False
        ret['comment'] = 'Error with volume {0}: {1}'.format(name, err)
        return ret


def linked(storage, container, name=None):
    ret = {'name': storage, 'result': None, 'comment': '', 'changes': {}}

    volume = __salt__['porto.storage_info'](storage)
    if volume:
        path = volume.keys()[0]

    """
        There is no API to know if volume is already linked
        without trying to do it. So we always return True with test=True.
    """
    if __opts__['test']:
        ret['result'] = True
        return ret

    ret['result'] = __salt__['porto.link_volume'](path, container)
    if ret['result']:
        ret['comment'] = 'Volume {0} has been linked to container {1}.'.format(storage, container)
        ret['changes'] = {'status': 'linked'}
    elif ret['result'] is None:
        ret['result'] = True
        ret['comment'] = 'Volume {0} is already linked to container {1}'.format(storage, container)
    else:
        ret['comment'] = 'Could not link volume {0} to container {1}.'.format(storage, container)

    return ret


def unlinked(storage, container, name=None):
    ret = {'name': storage, 'result': None, 'comment': '', 'changes': {}}

    volume = __salt__['porto.storage_info'](storage)
    if volume:
        path = volume.keys()[0]

    """
        There is no API to know if volume is already unlinked
        without trying to do it. So we always return True with test=True.
    """
    if __opts__['test']:
        ret['result'] = True
        return ret

    ret['result'] = __salt__['porto.unlink_volume'](path, container)
    if ret['result']:
        ret['comment'] = 'Volume {0} has been unlinked from container {1}.'.format(storage, container)
        ret['changes'] = {'status': 'unlinked'}
    elif ret['result'] is None:
        ret['result'] = True
        ret['comment'] = 'Volume {0} is already not linked to container {1}'.format(storage, container)
    else:
        ret['comment'] = 'Could not unlink volume {0} from container {1}.'.format(storage, container)

    return ret
