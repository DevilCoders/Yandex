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


__virtualname__ = 'porto_agent'


def __virtual__():
    if 'porto_agent.run' in __salt__:
        return __virtualname__
    else:
        return False


def run(command, container, flags, secrets, name=None):
    ret = {
        'name': "%s %s %s -t %s" % (__virtualname__, command, flags, container),
        'result': None,
        'comment': '',
        'changes': {},
    }

    flags += ['--show-changes']
    if __opts__['test']:
        flags += ['--dryrun']
    sec_json = ""
    if secrets:
        flags += ['--secrets']
        import json

        sec_json = json.dumps(secrets) + "\n"

    try:
        info = __salt__['porto_agent.run'](command=command, container=container, flags=flags, stdin=sec_json)
    except (CommandNotFoundError, CommandExecutionError) as err:
        ret['result'] = False
        ret['comment'] = 'Error in {0} state for: {1}: {2}'.format(command, name, err)
        return ret

    ret['comment'] = info['logs']
    ret['result'] = info['result']
    if info['out']:
        if ret['result'] and __opts__['test']:
            ret['result'] = None
        ret['changes'] = {'out': info['out']}
    else:
        ret['changes'] = {}
    return ret
