# -*- coding: utf-8 -*-
"""
Addition MDB states for managing windows
"""

from __future__ import absolute_import, print_function, unicode_literals

import logging
import os.path
import subprocess
import time
import salt.utils.platform

# Import 3rd Party Libraries
try:
    import pythoncom
    import win32api
    import win32com.client

    HAS_DEPENDENCIES = True
except ImportError:
    HAS_DEPENDENCIES = False

PATH_REGKEY = r'SYSTEM\CurrentControlSet\Control\Session Manager\Environment'
NSSM_BIN = r'C:\Program Files\NSSM\tools\win64\nssm'

# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}
__pillar__ = {}

log = logging.getLogger(__name__)


def __virtual__():
    if not salt.utils.platform.is_windows():
        return False, "State mdb_windows: Only available on Windows"
    if not HAS_DEPENDENCIES:
        return False, "State mdb_windows: Missing dependencies"
    return True


def nupkg_installed(name, version, source='MDB', keep_versions=5, stop_service=None):
    """
    Install/updates nupkg package from MDB repo
    """
    prog_dir = r'C:\Program Files'
    pkg_dir = r'C:\Program Files\PackageManagement\NuGet\Packages'
    # add zero minor if needed
    version = version + '.0' if version.count('.') == 1 else version
    pkg_path = os.path.join(pkg_dir, name + '.' + version)
    prog_path = os.path.join(prog_dir, name)

    args = {'name': name, 'version': version, 'prog_path': prog_path, 'pkg_path': pkg_path, 'source': source}

    version_cmd = 'Get-Package -Name "{name}" | Select Version'.format(**args)
    resolve_cmd = 'Get-Item -Path "{prog_path}" -ErrorAction:SilentlyContinue | Select Target'.format(**args)
    install_cmd = 'Install-Package -Source "{source}" -Name "{name}" -RequiredVersion "{version}"'.format(**args)
    symlink_cmd = 'New-Item -ItemType SymbolicLink -Force -Path "{prog_path}" -Target "{pkg_path}"'.format(**args)
    cleanup_cmd = (
        'Get-Package -Name "{name}" -AllVersions '.format(**args)
        + ' | Sort -Property @{Expression={[System.Version]$_.Version}; Descending=$true} '
        + ' | Select -Skip {keep_versions} | Uninstall-Package'.format(keep_versions=keep_versions)
    )
    # this is workaround to delete previos (directly installed) packages, delete after MDB-10719 deployed
    workaround_cmd = (
        'Get-Item -Path "{prog_path}" -ErrorAction:SilentlyContinue '.format(**args)
        + ' | Where { $_.LinkType -eq $null } | Remove-Item -Recurse -Force'
    )
    if stop_service:
        preinstall_cmd = '''Stop-Service -Name {stop_service} -ErrorAction SilentlyContinue;
        '''.format(
            stop_service=stop_service
        )
        install_cmd = preinstall_cmd + install_cmd
    ok, out, err = __salt__['mdb_windows.run_ps'](version_cmd)
    if ok:
        old_version = out.get('Version', '')
        old_version = old_version + '.0' if old_version.count('.') == 1 else old_version
    else:
        old_version = ''

    ok, out, err = __salt__['mdb_windows.run_ps'](resolve_cmd)
    if ok:
        old_symlink = (out.get('Target') or [''])[0]
        old_symlink = old_symlink.rstrip(os.path.sep)
    else:
        old_symlink = ''

    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    if version == old_version and pkg_path == old_symlink:
        ret['result'] = True
        return ret

    ret['changes'].update({name: {'old': old_version, 'new': version}})

    if __opts__['test']:
        ret['result'] = None
    else:
        ok, _, err = __salt__['mdb_windows.run_ps'](install_cmd)
        if not ok:
            log.error('failed to install nupkg: %s', err)
            ret['result'] = False
            ret['changes'] = {}
            ret['comment'] = err
            return ret
        ok, _, err = __salt__['mdb_windows.run_ps'](workaround_cmd)
        if not ok:
            log.warn('failed to delete old package (workaround): %s', err)
        ok, _, err = __salt__['mdb_windows.run_ps'](symlink_cmd)
        if not ok:
            log.error('failed to update package symlink: %s', err)
            ret['result'] = False
            ret['changes'] = {}
            ret['comment'] = err
            return ret
        ok, _, err = __salt__['mdb_windows.run_ps'](cleanup_cmd)
        if not ok:
            log.warn('failed to cleanup old package version: %s', err)
    ret['result'] = True
    return ret


def service_installed(name, service_name, service_call, service_args, service_settings=None):
    """
    Function creates a windows service from an executable by using NSSM
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }
    if not service_args:
        service_args = []

    installed = True
    setting_changes = {}
    try:
        subprocess.check_output([NSSM_BIN, 'status', service_name])
    except Exception:
        installed = False
    if service_settings:
        if installed:
            for setting, val in service_settings.items():
                current_val = ''
                try:
                    current_val = subprocess.check_output([NSSM_BIN, 'get', service_name, setting])
                    current_val = (current_val or '').decode('UTF-16').strip()
                except Exception:
                    log.warn("failed to get setting %s of service %s", setting, service_name)
                if current_val != val:
                    setting_changes[setting] = val
        else:
            setting_changes = service_settings

    if installed and not setting_changes:
        ret['changes'] = {}
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['changes'] = {name: "service {service_name} set to be installed/updated".format(service_name=service_name)}
        ret['result'] = None
        return ret

    output = ''
    if not installed:
        try:
            output = subprocess.check_output(
                [NSSM_BIN, 'install', service_name, service_call] + service_args, stderr=subprocess.STDOUT
            )
            ret['result'] = True
            ret['changes'] = {name: "service {service_name} installed".format(service_name=service_name)}
        except Exception:
            ret['result'] = False
            ret['comment'] = output
            return ret
    if setting_changes:
        for setting, val in setting_changes.items():
            try:
                if val:
                    output = subprocess.check_output(
                        [NSSM_BIN, 'set', service_name, setting, val], stderr=subprocess.STDOUT
                    )
                else:
                    output = subprocess.check_output(
                        [NSSM_BIN, 'reset', service_name, setting], stderr=subprocess.STDOUT
                    )
                ret['changes'].setdefault(name, "service {service_name} updated".format(service_name=service_name))
            except Exception:
                ret['result'] = False
                ret['comment'] = output
                return ret

    return ret


def service_settings(name, service_name, display_name=None, start_type=None):
    """
    State allows to set attributes of a windows service such as display name and start type
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }
    ok, out, err = __salt__['mdb_windows.run_ps']('Get-Service {} | Select *'.format(service_name))
    STARTUP_TYPES = {
        'Boot': 0,
        'System': 1,
        'Automatic': 2,
        'Manual': 3,
        'Disabled': 4,
    }
    changes = {}
    if display_name is not None and out.get('DisplayName') != display_name:
        changes['DisplayName'] = display_name
    if start_type is not None and out.get('StartType') != STARTUP_TYPES.get(start_type):
        changes['StartupType'] = start_type

    if not changes:
        ret['changes'] = {}
        ret['result'] = True
    elif __opts__['test']:
        ret['changes'] = {name: "{} settings need to be changed: {}".format(service_name, changes)}
        ret['result'] = None
    else:
        cmd = 'Set-Service {}'.format(service_name)
        for opt, val in changes.items():
            cmd += ' -{} {}'.format(opt, val)
        ok, out, err = __salt__['mdb_windows.run_ps'](cmd)
        ret['result'] = ok
        if ok:
            ret['changes'] = {name: "{} settings updated: {}".format(service_name, changes)}
        else:
            ret['comment'] = err
    return ret


def service_stopped(name, service_name):
    """
    State allows to ensure windows service is in the stopped state
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    ok, out, err = __salt__['mdb_windows.run_ps']('Get-Service {} | Select Status'.format(service_name))
    stopped = out.get('Status') == 1  # "Stopped"

    if stopped:
        ret['changes'] = {}
        ret['result'] = True
    else:
        if __opts__['test']:
            ret['changes'] = {name: "{} set to stop".format(service_name)}
            ret['result'] = None
        else:
            ok, out, err = __salt__['mdb_windows.run_ps']('Stop-Service -Force {}'.format(service_name))
            ret['result'] = ok
            if ok:
                ret['changes'] = {name: "{} stopped".format(service_name)}
            else:
                ret['comment'] = err
    return ret


def service_running(name, service_name):
    """
    State unsures the windows service is in the running state
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    ok, out, err = __salt__['mdb_windows.run_ps']('Get-Service {} | Select Status'.format(service_name))
    running = out.get('Status') == 4  # "Running"

    if running:
        ret['changes'] = {}
        ret['result'] = True
    else:
        if __opts__['test']:
            ret['changes'] = {name: "{} set to start".format(service_name)}
            ret['result'] = None
        else:
            ok, out, err = __salt__['mdb_windows.run_ps']('Start-Service {}'.format(service_name))
            ret['result'] = ok
            if ok:
                ret['changes'] = {name: "{} started".format(service_name)}
            else:
                ret['comment'] = err
    return ret


def service_restarted(name, service_name):
    """
    State allow to restart the service
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    if __opts__['test']:
        ret['changes'] = {}
        # result should be None because we will restart service anyway
    else:
        ok, _, err = __salt__['mdb_windows.run_ps']('Restart-Service {}'.format(service_name))
        ret['result'] = ok

        if ok:
            ret['changes'] = {name: "{} service restarted successfully".format(service_name)}
        else:
            ret['comment'] = err

    return ret


def add_to_system_path(name, path):
    """
    State makes sure that the argument "path" is found in the PATH variable
    """
    import _winreg

    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }
    with _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, PATH_REGKEY, 0, _winreg.KEY_ALL_ACCESS) as reg_key:
        system_path = _winreg.QueryValueEx(reg_key, 'Path')[0].split(';')
        if path in system_path:
            ret['changes'] = {}
            ret['result'] = True
        else:
            if __opts__['test']:
                ret['result'] = None
                ret['changes'] = {name: "path will be added: {path}".format(path=path)}
            else:
                system_path.append(path)
                _winreg.SetValueEx(reg_key, 'Path', None, _winreg.REG_EXPAND_SZ, ';'.join(system_path))
                ret['result'] = True
                ret['changes'] = {name: "path added: {path}".format(path=path)}
    return ret


def os_clustered(name):
    """
    State ensures that the OS cluster exists and creates it if needed.
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }
    ok, out, err = __salt__['mdb_windows.run_ps']('get-service clussvc|select Status')

    if out.get('Status', '') == 4:
        ret['result'] = True
        ret['comment'] = 'OS cluster present'
    else:
        if __opts__['test']:
            ret['changes'] = {name: 'OS to be clustered'}
        else:
            ok, out, err = __salt__['mdb_windows.run_ps'](
                'New-Cluster -Name "' + str(name) + '" -Node . -NoStorage -AdministrativeAccessPoint DNS -Force'
            )
            if ok:
                ret['changes'] = {name: 'Cluster created'}
                ret['result'] = True
            else:
                ret['result'] = False
                ret['comment'] = err
    return ret


def node_present(name, timeout=300, interval=20):
    """
    States ensures that all the cluster nodes are added to Windows cluster.
    Can be run on a cluster member only.
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    nodes, err = __salt__['mdb_windows.get_cluster_nodes']()
    if err is not None:
        ret['result'] = False
        ret['comment'] = err
        return ret

    # we shorten the name to 15 symbols if it is longer.
    node = __salt__['mdb_windows.shorten_hostname'](name.split('.')[0])

    if node in nodes and nodes[node] != 'Down':
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['changes'] = {name: 'should be added to cluster'}
        ret['result'] = None
        return ret

    ready = __salt__['mdb_windows.wait_host'](name, port=445, timeout=timeout, interval=interval)
    if not ready:
        ret['result'] = False
        ret['comment'] = '{} did not became ready within {}s'.format(name, timeout)
        return ret

    if nodes.get(node) == 'Down':
        ok, out, err = __salt__['mdb_windows.run_ps']('Remove-ClusterNode -Force {}'.format(node))
        if not ok:
            ret['result'] = False
            ret['comment'] = 'failed to removed staled cluster node {}: {}'.format(name, err)
            return ret

    ok, out, err = __salt__['mdb_windows.run_ps']('Add-ClusterNode {}'.format(node))
    if ok:
        ret['result'] = True
        ret['changes'] = {name: 'added to cluster'}
        return ret
    else:
        # TODO: double check if someone else added this node to cluster
        nodes, _ = __salt__['mdb_windows.get_cluster_nodes']()
        if nodes and node in nodes:
            ret['result'] = True
            ret['changes'] = {name: 'added to cluster (by another node)'}
        else:
            ret['result'] = False
            ret['comment'] = err
    return ret


def wait_node_joined(name, timeout=60, interval=10):
    """
    Waits until node added to cluster
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }
    node = __salt__['mdb_windows.shorten_hostname'](name.split('.')[0])
    deadline = time.time() + timeout
    while time.time() < deadline:
        nodes, _ = __salt__['mdb_windows.get_cluster_nodes']()
        if node in nodes:
            ret['result'] = True
            return ret
        else:
            log.debug("node %s is not yet joined the cluster", name)
            time.sleep(interval)
    ret['result'] = False
    ret['comment'] = 'node {} did not joined cluster with {}s'.format(name, timeout)
    return ret


def fw_netbound_rule_present(
    name,
    localport='any',
    interface=None,
    protocol="tcp",
    action="allow",
    dir="in",
    remoteip="any",
    remoteport="any",
    enabled=True,
):
    """
    State ensures existence of windows firewall rule with advanced filtering acapabilities.
    All rules options are checked for match so it consumes significant time. Use only when it is really needed.
    """
    ret = {"name": name, "result": True, "changes": {}, "comment": ""}

    rule_direction = {1: 'in', 2: 'out'}
    rule_direction_r = {v: k for k, v in rule_direction.items()}

    rule_protocol = {6: 'tcp', 17: 'udp', 1: 'icmpv4', 58: 'icmpv6'}
    rule_protocol_r = {v: k for k, v in rule_protocol.items()}

    rule_action = {0: 'block', 1: 'allow'}
    rule_action_r = {v: k for k, v in rule_action.items()}

    rule_any = {'*': 'any'}
    rule_any_r = {v: k for k, v in rule_any.items()}

    # format remoteip string

    if isinstance(remoteip, str):
        remoteip = [remoteip]

    if 'any' not in remoteip:
        ip_list2 = []
        for i in remoteip:
            if '/' in i:  # pass subnet as-is
                pass
            elif ':' not in i:  # ipv4 address needs a mask /255.255.255.255
                i = i + '/255.255.255.255'
            else:
                if '%' in i:  # need to exclude interface index
                    # (e.g. %5 in the end) from ipv6 address
                    i = i[0 : i.find('%')]
                i = '{}-{}'.format(i, i)
            ip_list2 += [i]
        remoteip = ','.join(sorted(ip_list2))
    # translating parameters to what COM object expects
    com_action = rule_action_r.get(action)
    com_direction = rule_direction_r.get(dir)
    com_remoteip = rule_any_r.get(remoteip, remoteip)
    com_remoteport = rule_any_r.get(remoteport, remoteport)
    com_localport = rule_any_r.get(localport, localport)
    com_interface = [interface]
    com_protocol = rule_protocol_r.get(protocol)

    try:
        fw = win32com.client.Dispatch("HNetCfg.FwPolicy2")
    except:
        ret['result'] = False
        ret['comment'] = 'Could not get firewall rule data'
        return ret
    all_rules = [r.Name for r in fw.Rules]

    if name in all_rules:
        rule = fw.Rules.Item(name)
        rule_data = {
            'action': rule.Action,
            'direction': rule.Direction,
            'remote_address': rule.RemoteAddresses,
            'local_ports': rule.LocalPorts,
            'remote_ports': rule.RemotePorts,
            'protocol': rule.Protocol,
            'interface': rule.Interfaces,
            'enabled': rule.Enabled,
        }
        if rule_data['interface']:
            rule_data['interface'] = rule_data['interface'][0]
        if rule_data['protocol'] in [1, 58]:  # icmpv4 and icmpv6
            rule_data['local_ports'] = '*'
            rule_data['remote_ports'] = '*'

        p_enabled = rule_data.get('enabled')
        p_action = rule_action.get(rule_data.get('action'))
        p_direction = rule_direction.get(rule_data.get('direction'))
        p_remoteip = str(rule_any.get(rule_data.get('remote_address'), rule_data.get('remote_address')))
        p_remote_port = str(rule_any.get(rule_data.get('remote_ports'), rule_data.get('remote_ports')))
        p_local_port = str(rule_any.get(rule_data.get('local_ports'), rule_data.get('local_ports')))
        if rule_data.get('interface'):
            p_interface = str(rule_data.get('interface'))
        else:
            p_interface = None

        p_protocol = str(rule_protocol.get(rule_data.get('protocol'), rule_data.get('protocol')))

        b_enabled = p_enabled != enabled
        b_action = p_action != action
        b_direction = p_direction != dir
        b_remoteip = p_remoteip != remoteip
        b_remote_port = p_remote_port != str(remoteport)
        b_local_port = p_local_port != str(localport)
        b_interface = p_interface != interface
        b_protocol = p_protocol != protocol

        if (
            b_enabled
            or b_action
            or b_direction
            or b_remoteip
            or b_remote_port
            or b_local_port
            or b_interface
            or b_protocol
        ):

            if __opts__['test']:
                ret['changes'][name] = "some rules property requires changes"
                ret['result'] = None
                return ret
            changes_str = ''
            try:
                if b_enabled:
                    rule.Enabled = enabled
                    changes_str += 'enabled: {0} -> {1}; '.format(p_enabled, enabled)
                if b_action:
                    rule.Action = com_action
                    changes_str += 'action: {0} -> {1}; '.format(p_action, action)
                if b_direction:
                    rule.Direction = com_direction
                    changes_str += 'direction: {0} -> {1}; '.format(p_direction, dir)
                if b_remoteip:
                    rule.RemoteAddresses = com_remoteip
                    changes_str += 'remoteip: {0} -> {1}; '.format(p_remoteip, remoteip)
                if b_remote_port and protocol not in ['icmpv4', 'icmpv6']:
                    rule.RemotePorts = com_remoteport
                    changes_str += 'remoteport: {0} -> {1}; '.format(p_remote_port, remoteport)
                if b_local_port and protocol not in ['icmpv4', 'icmpv6']:
                    rule.LocalPorts = com_localport
                    changes_str += 'local_port: {0} -> {1}; '.format(p_local_port, localport)
                if b_interface:
                    if interface:
                        rule.Interfaces = [interface]
                    else:
                        rule.Interfaces = []
                    changes_str += 'interface: {0} -> {1}; '.format(p_interface, interface)
                if b_protocol:
                    rule.Protocol = com_protocol
                    changes_str += 'protocol: {0} -> {1}; '.format(p_protocol, protocol)
                ret['result'] = True
                ret['changes'][name] = 'Altered:' + changes_str
                ret['comment'] = 'Rule has been altered'
            except pythoncom.com_error as error:
                hr, msg, exc, arg = error.args  # pylint: disable=W0633
                try:
                    failure_code = (win32api.FormatMessage(exc[5])).replace('\r\n', '')
                except KeyError:
                    failure_code = 'Unknown Failure: {0}'.format(error)
                ret["comment"] = 'Could not alter rule: {0}'.format(failure_code)
                ret["result"] = False
                return ret
            return ret
        ret['result'] = True
        ret['comment'] = 'Rule ok'
        return ret

    if __opts__['test']:
        ret['changes'][name] = 'Rule needs to be added'
        ret['result'] = None
        return ret
    try:
        new_rule = win32com.client.Dispatch("HNetCfg.FWRule")
        new_rule.Enabled = enabled
        new_rule.Name = name
        new_rule.Protocol = com_protocol
        new_rule.Direction = com_direction
        if protocol not in ['icmpv4', 'icmpv6']:
            new_rule.LocalPorts = com_localport
            new_rule.RemotePorts = com_remoteport
        new_rule.RemoteAddresses = com_remoteip
        new_rule.Action = com_action
        if interface:
            new_rule.Interfaces = com_interface
        fw.Rules.Add(new_rule)
        ret['result'] = True
        ret['changes'][name] = 'Present'
    except pythoncom.com_error as error:
        hr, msg, exc, arg = error.args  # pylint: disable=W0633
        try:
            failure_code = (win32api.FormatMessage(exc[5])).replace('\r\n', '')
        except (KeyError, TypeError):
            failure_code = 'Unknown Failure: {0}'.format(error)
        ret["comment"] = 'Could not add rule: {0}'.format(failure_code)
        ret["result"] = False
    return ret


def service_recovery_set(name, recovery_action='restart', recovery_delay_ms=60000, reset_delay_s=60):
    ret = {"name": name, "result": True, "changes": {}, "comment": ""}
    service_recovery_props = __salt__['mdb_windows.get_service_recovery_info'](name)
    if service_recovery_props.get('FAILURE_ACTIONS', ''):
        if service_recovery_props['RESET_PERIOD (in seconds)'] == str(reset_delay_s):
            failure_actions = service_recovery_props['FAILURE_ACTIONS'][0].lower().split(' ')
            if recovery_action in failure_actions:
                if str(recovery_delay_ms) in failure_actions:
                    ret['result'] = True
                    ret['comment'] = 'Recovery action is OK for service {}'.format(name)
                    return ret
    if __opts__['test']:
        ret['result'] = None
        ret['changes'][name] = 'Service recovery properties need to be set'
        return ret
    try:
        cmd = 'sc.exe failure {} reset= {} actions= {}/{}'.format(
            name, reset_delay_s, recovery_action, recovery_delay_ms
        )
        subprocess.check_call(cmd)
        ret['result'] = True
        ret['changes'][name] = 'Service recovery actions adjusted'
        return ret
    except subprocess.CalledProcessError as e:
        msg = "Failed to modify service recovery options for {}: {}".format(name, e)
        log.error(msg)
        ret['result'] = False
        ret['comment'] = msg
        return ret


def wait_cluster_resources_up(name, resources=[], timeout=60, interval=10):
    """
    Waits until cluster resource becomes online
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    nodes = __salt__['pillar.get']('data:dbaas:shard_hosts')
    if len(nodes) == 1:
        ret['result'] = True
        ret['comment'] = 'standalone instance'
        return ret

    deadline = time.time() + timeout
    while time.time() < deadline:
        res, _ = __salt__['mdb_windows.get_cluster_resources'](resources)
        if len((res.values())) == (res.values()).count(u'Online'):
            ret['result'] = True
            return ret
        if u'Failed' in res.values():
            ret['result'] = False
            ret['comment'] = 'Some resources are in a Failed state'
            return ret
        else:
            log.debug("Cluster resources are not yet operational")
            time.sleep(interval)
    ret['result'] = False
    ret['comment'] = 'Cluster resources did not come online within {}s'.format(timeout)
    return ret
