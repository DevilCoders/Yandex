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


__virtualname__ = 'porto_container'


def __virtual__():
    if 'porto.containers_list' in __salt__:
        return __virtualname__
    else:
        return False


ALLOWED_PROPERTIES = (
    'aging_time',
    'anon_limit',
    'bind',
    'capabilities',
    'capabilities_ambient',
    'command',
    'controllers',
    'cpu_guarantee',
    'cpu_limit',
    'cpu_period',
    'cpu_policy',
    'cpu_set',
    'cpu_weight',
    'cwd',
    'default_gw',
    'devices',
    'dirty_limit',
    'enable_porto',
    'env',
    'group',
    'hostname',
    'hugetlb_limit',
    'io_limit',
    'io_ops_limit',
    'io_policy',
    'io_weight',
    'ip',
    'ip_limit',
    'isolate',
    'max_respawns',
    'memory_guarantee',
    'memory_limit',
    'net',
    'net_guarantee',
    'net_limit',
    'net_rx_limit',
    'oom_is_fatal',
    'oom_score_adj',
    'owner_group',
    'owner_user',
    'place',
    'place_limit',
    'porto_namespace',
    'pressurize_on_death',
    'private',
    'recharge_on_pgfault',
    'resolv_conf',
    'respawn',
    'root',
    'root_readonly',
    'stderr_path',
    'stdin_path',
    'stdout_limit',
    'stdout_path',
    'sysctl',
    'thread_limit',
    'ulimit',
    'umask',
    'user',
    'virt_mode',
    'volumes_required',
    'weak',
)


def present(name, storage=None, virt_mode='os', cwd='/', net='L3 eth0', hostname=None, **kwargs):
    ret = {'name': name, 'result': None, 'comment': '', 'changes': {}}

    if 'project_id' in kwargs:
        project_id = kwargs.pop('project_id')
    else:
        project_id = ''

    properties = {
        'virt_mode': virt_mode,
        'cwd': cwd,
        'net': net,
        'oom_is_fatal': False,
    }
    if hostname is None:
        properties['hostname'] = name
    else:
        properties['hostname'] = hostname

    for k, v in kwargs.items():
        if k in ALLOWED_PROPERTIES:
            properties[k] = v

    if 'anon_limit' not in properties and 'memory_limit' in properties:
        properties['anon_limit'] = str(int(float(properties['memory_limit']) * 0.95))

    if 'root' not in properties and storage:
        try:
            properties['root'] = __salt__['porto.storage_info'](storage).keys()[0]
        except IndexError:
            if __opts__['test']:
                pass

    if 'ip' not in properties:
        try:
            iface_mask = 'eth'
            if __salt__['pillar.get']('data:use_vlan688'):
                iface_mask = 'vlan688'

            found = False
            for intf, addr in __salt__['grains.get']('ip6_interfaces').items():
                if not intf.startswith(iface_mask):
                    continue
                if found:
                    break
                for address in addr:
                    if address.startswith('fe80'):
                        continue
                    network = ':'.join(address.split(':')[:4])
                    found = True
                    break

            import hashlib

            tail = hashlib.md5(name).hexdigest()[-8:]
            tail = tail[:4] + ':' + tail[4:]

            if project_id:
                container_ip = network + ':' + project_id + ':' + tail
            else:
                container_ip = network + '::' + tail
            properties['ip'] = 'eth0 %s' % container_ip
        except Exception:
            pass

    if 'oom_is_fatal' not in properties:
        properties['oom_is_fatal'] = False

    if 'capabilities' not in properties:
        properties['capabilities'] = (
            'CHOWN;DAC_OVERRIDE;FOWNER;FSETID;KILL;SETGID;SETUID;SETPCAP;LINUX_IMMUTABLE;NET_BIND_SERVICE;'
            'NET_ADMIN;NET_RAW;IPC_LOCK;SYS_CHROOT;SYS_PTRACE;SYS_BOOT;SYS_NICE;SYS_RESOURCE;MKNOD;AUDIT_WRITE;SETFCAP'
        )

    try:
        container = __salt__['porto.container_info'](name)
        if container:
            current = container[name]
            need_update = {}

            for key, value in current.items():
                if key in properties and value != properties[key]:
                    need_update[key] = properties[key]

            if need_update:
                if __opts__['test']:
                    ret['result'] = None
                    ret['comment'] = 'Container {0} will be changed.'.format(name)
                else:
                    container = __salt__['porto.tune_container'](name, **need_update)
                    if container is None:
                        # Some of changed properties can't be changed in runtime.
                        state = __salt__['porto.container_state'](name)
                        if 'running' not in state:
                            raise Exception('Container is not running.')
                        ret['result'] = __salt__['porto.container_action'](name, 'stop')
                        if ret['result']:
                            container = __salt__['porto.tune_container'](name, **properties)
                            ret['comment'] = 'Container {0} has'.format(name) + ' been stopped and changed.'
                    else:
                        ret['result'] = True
                        ret['comment'] = 'Container {0} has been changed.'.format(name)

                ret['changes'] = need_update
            else:
                ret['result'] = True
                ret['comment'] = 'Container {0} is in actual state.'.format(name)
        else:
            if __opts__['test']:
                ret['result'] = None
                ret['comment'] = 'Container {0} will be created.'.format(name)
            else:
                container = __salt__['porto.create_container'](name)
                container = __salt__['porto.tune_container'](name, **properties)
                ret['result'] = True
                ret['comment'] = 'Container {0} has been created.'.format(name)

            ret['changes'] = properties

        return ret
    except (CommandNotFoundError, CommandExecutionError) as err:
        ret['result'] = False
        ret['comment'] = 'Error with container {0}: {1}'.format(name, err)
        return ret


def running(name):
    ret = {'name': name, 'result': None, 'comment': '', 'changes': {}}

    try:
        container = __salt__['porto.container_info'](name)
        if not container:
            ret['result'] = False
            ret['comment'] = 'Container {0} does not exist.'.format(name)
            return ret

        state = __salt__['porto.container_state'](name)
        if 'running' in state:
            ret['result'] = True
            ret['comment'] = 'Container {0} is running.'.format(name)
            return ret

        if __opts__['test']:
            ret['result'] = None
            ret['comment'] = 'Container {0} will be started.'.format(name)
        else:
            ret['result'] = __salt__['porto.container_action'](name, 'start')
            ret['comment'] = 'Container {0} has been started.'.format(name)
            ret['changes'] = {'action': 'start', 'state': __salt__['porto.container_state'](name)}

        return ret
    except (CommandNotFoundError, CommandExecutionError) as err:
        ret['result'] = False
        ret['comment'] = 'Error with container {0}: {1}'.format(name, err)
        return ret


def absent(name):
    ret = {'name': name, 'result': None, 'comment': '', 'changes': {}}

    try:
        container = __salt__['porto.container_info'](name)
        if not container:
            ret['result'] = True
            ret['comment'] = 'Container {0} does not exist.'.format(name)
            return ret

        if __opts__['test']:
            ret['result'] = None
            ret['comment'] = 'Container {0} will be destroyed.'.format(name)
        else:
            ret['result'] = __salt__['porto.destroy_container'](name)
            ret['comment'] = 'Container {0} has been destroyed.'.format(name)
            ret['changes'] = {'action': 'destroy'}

        return ret
    except (CommandNotFoundError, CommandExecutionError) as err:
        ret['result'] = False
        ret['comment'] = 'Error with container {0}: {1}'.format(name, err)
        return ret
