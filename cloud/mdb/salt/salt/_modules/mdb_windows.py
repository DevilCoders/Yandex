# -*- coding: utf-8 -*-
"""
Additional MDB functions for managing windows
"""
from __future__ import absolute_import, print_function, unicode_literals

import json
import logging
import socket
import subprocess
import time
import salt.utils.platform

try:
    from salt.utils.network import host_to_ips as _host_to_ips
    from salt.modules.win_service import modify
    from salt.modules.win_useradd import setpassword

    HAS_DEPENDENCIES = True
except ImportError:
    HAS_DEPENDENCIES = False

try:
    import six
except ImportError:
    from salt.ext import six


def __virtual__():
    if not salt.utils.platform.is_windows():
        return False, "Module mdb_windows: Only available on Windows"
    if not HAS_DEPENDENCIES:
        return False, "Module mdb_windows: Missing dependencies"
    return True


log = logging.getLogger(__name__)


CLUSTER_NODE_STATE_MAP = {
    0: 'Up',
    1: 'Down',
    2: 'Paused',
    3: 'Joining',
}

CLUSTER_RESOURCE_STATE_MAP = {
    1: 'Initializing',
    2: 'Online',
    3: 'Offline',
    4: 'ResourceFailed',
    128: 'Pending',
    129: 'OnlinePending',
    130: 'OfflinePending',
    -1: 'Unknown',
}


def run_ps(cmd):
    """
    function allows to run a Powershell script and return a result as json.
    Script has to return at least something or it will break down.

    example:
        ok, out , err = run_ps('Get-Process')
    """
    cmd = '$WarningPreference = "SilentlyContinue"; {cmd} | ConvertTo-Json'.format(cmd=cmd.strip())
    log.debug("PS RUN: %s", cmd)
    proc = subprocess.Popen(
        ['powershell', cmd], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True
    )
    out, err = proc.communicate()
    if out is not None:
        out = ensure_str(out)
    if err is not None:
        err = ensure_str(err)

    log.debug("PS RESULT: %s", out)
    log.debug("PS ERROR: %s", err)
    try:
        out = json.loads(out or '{}')
    except ValueError:
        log.error("Failed to parse JSON from powershell: \n" + str(out))
        raise
    return proc.returncode == 0, out, err


def get_fqdn():
    """
    Function returns Windows hosts FQDN
    """
    cmd = "[System.Net.Dns]::GetHostByName($env:computerName)|Select HostName -ExpandProperty HostName"
    ok, out, err = run_ps(cmd)
    return out['value']


def restart_service(name, force=False):
    """
    Restarts selected windows service.
    Force option means restart with dependent services
    """
    if force:
        force = '-Force'
    cmd = "Restart-Service '{name}' {force} -WarningAction Ignore".format(name=name, force=force)
    ok, out, err = run_ps(cmd)
    if ok:
        return True
    else:
        return err


def shorten_hostname(name):
    """
    Shorten the hostname to 15 symbols with respect to windows naming rules
    """
    domain = ''
    if '.' in name:
        fqdn = name
        name = fqdn.split('.')[0]
        domain = fqdn[len(name) : len(fqdn)]
    if len(name) > 15:
        num = 15
        while num > 7 and not name[len(name) - num].isalnum():
            num -= 1
        name = name[len(name) - num : len(name)]
    return name + domain


def wait_host(name, port, timeout=60, interval=10):
    """
    Waits for a host availability by a specific port during specified number of seconds
    """
    deadline = time.time() + timeout
    while time.time() < deadline:
        sct = None
        try:
            sct = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sct.connect((name, port))
        except Exception as err:
            log.debug("failed to connect to %s:%s %s", name, port, err)
            time.sleep(interval)
        else:
            return True
        finally:
            if sct is not None:
                sct.close()
    return False


def get_cluster_nodes():
    """
    Returns list of cluster node names or error
    """
    ok, nodes, err = __salt__['mdb_windows.run_ps']('Get-ClusterNode | Select Name,State')
    if not ok:
        return {}, (err or "Failed to list cluster nodes")
    if isinstance(nodes, dict):  # 1 item
        nodes = [nodes]
    nodes = {n['Name']: CLUSTER_NODE_STATE_MAP.get(int(n['State']), 'Unknown') for n in nodes}
    return nodes, None


def get_cluster_nodes_ip(exclude_self=False):
    """
    Returns a string of ip addresses
        if exclude_self = True then addressed of local node is excluded
    """
    nodes = __salt__['pillar.get']('data:dbaas:shard_hosts', {})
    myself = get_fqdn()
    ip_list = []

    for node in nodes:
        if node != myself or not exclude_self:
            ip = _host_to_ips(node)
            if isinstance(ip, list):
                ip_list += ip
            else:
                ip_list += [ip]
    return ip_list


def set_administrator_password(user='.\\Administrator'):
    """
    Sets the salt-minion windows service action account
    Service restart is not included and should be managed externally.
    No use in highstate. Only direct salt-call usage
    """
    # check if named account exists in pillar

    passwd = __salt__['pillar.get']('data:windows:users:{}:password'.format(user.replace('.\\', '')))
    if not passwd:
        return False

    pwd_set = setpassword(user.replace('.\\', ''), passwd)
    if not pwd_set:
        return False

    svc_mod = modify(name='salt-minion', account_name=user, account_password=passwd)
    if not svc_mod:
        return False

    return True


def get_service_recovery_info(service_name):
    """
    returns the parsed output of sc qfailure <service_name>
    """
    try:
        output = subprocess.check_output("sc.exe qfailure {}".format(service_name), shell=True)
    except subprocess.CalledProcessError as err:
        msg = "Failed to get service {} recovery properties: {}".format(service_name, err)
        log.error(msg)
        raise
    result = {}
    failure_actions = []
    for row in output.split('\n'):
        if ': ' in row:
            key, value = row.split(': ', 1)
            result[key.strip()] = value.strip()
            if key.strip() == 'FAILURE_ACTIONS':
                failure_actions.append(value)
        else:
            if failure_actions != {} and ': ' not in row and 'milliseconds.' in row:
                failure_actions.append(row.strip())
    result['FAILURE_ACTIONS'] = failure_actions
    return result


def is_witness(fqdn):
    """
    Returns True if node is a witness node.
    """
    cluster = __salt__['pillar.get']('data:dbaas:cluster:subclusters')
    for sc_name, sc in cluster.items():
        if "windows_witness" in sc['roles'] and fqdn in sc['hosts'].keys():
            return True
    return False


def get_hosts_by_role(role):
    """
    Returns a list of hosts with specific role
    """
    ret = []
    cluster = __salt__['pillar.get']('data:dbaas:cluster:subclusters')
    if not isinstance(cluster, dict):
        return {}
    for sc_name, sc in cluster.items():
        if role in sc['roles']:
            ret += sc['hosts'].keys()
    return ret


def get_cluster_resources(resources=[], verbose=False):
    """
    Returns list of cluster resources
    """
    if not isinstance(resources, list):
        resources = [resources]
    if verbose:
        cmd = 'Get-ClusterResource {}|Select Name, State'.format(','.join(resources))
    else:
        cmd = 'Get-ClusterResource {}|Select *'.format(','.join(resources))
    ok, res, err = __salt__['mdb_windows.run_ps'](cmd)
    if not ok:
        return {}, (err or "Failed to get cluster resources")
    if not isinstance(res, list):
        res = [res]
    if not verbose:
        res = {r['Name']: CLUSTER_RESOURCE_STATE_MAP.get(int(r['State']), 'Unknown') for r in res}
    return res, None


def ensure_str(s, encoding='utf-8', errors='strict'):
    """Coerce *s* to `str`.
    For Python 2:
      - `unicode` -> encoded to `str`
      - `str` -> `str`
    For Python 3:
      - `str` -> `str`
      - `bytes` -> decoded to `str`
    NOTE: The function equals to six.ensure_str that is not present in the salt version of six module.
    """
    if type(s) is str:
        return s
    if six.PY2 and isinstance(s, six.text_type):
        return s.encode(encoding, errors)
    elif six.PY3 and isinstance(s, six.binary_type):
        return s.decode(encoding, errors)
    elif not isinstance(s, (six.text_type, six.binary_type)):
        raise TypeError("not expecting type '%s'" % type(s))
    return s
