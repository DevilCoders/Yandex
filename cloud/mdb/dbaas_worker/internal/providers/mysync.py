"""
Mysync interaction module
"""

import datetime
import json
import os
import socket
import time
from typing import Dict, Any, Optional

import dateutil.parser
from kazoo.exceptions import NodeExistsError, NoNodeError

from dbaas_common import tracing

from ..exceptions import ExposedException
from ..utils import get_absolute_now
from .common import BaseProvider, Change
from .zookeeper import KAZOO_RETRIES, KazooContext


class MySyncError(ExposedException):
    """
    Base mysync interaction error
    """


def _mk_path(cid, *path):
    return os.path.join('mysql', cid, *path)


@KAZOO_RETRIES
@tracing.trace('MySync _GetMaster')
def _get_master(client, cid):
    """
    Get current master
    """
    tracing.set_tag('cluster.id', cid)
    return json.loads(client.get(_mk_path(cid, 'master'))[0].decode('utf-8'))


@KAZOO_RETRIES
@tracing.trace('MySync _GetHaNodes')
def _get_ha_nodes(client, cid):
    """
    Get list of HA nodes
    """
    tracing.set_tag('cluster.id', cid)
    try:
        return client.get_children(_mk_path(cid, 'ha_nodes'))
    except NoNodeError:
        return []


@KAZOO_RETRIES
@tracing.trace('MySync _GetActiveNodes')
def _get_active_nodes(client, cid):
    """
    Get current list of active nodes
    """
    tracing.set_tag('cluster.id', cid)
    try:
        return json.loads(client.get(_mk_path(cid, 'active_nodes'))[0].decode('utf-8'))
    except NoNodeError:
        return []


@tracing.trace('MySync _Switchover')
def _switchover(client, cid, old_master, new_master, timeout=180):
    """
    Initiate switchover and wait for completion
    """
    tracing.set_tag('cluster.id', cid)
    tracing.set_tag('mysync.master.old', old_master)
    tracing.set_tag('mysync.master.new', new_master)

    path = _mk_path(cid, 'switch')
    switchover = {
        'from': old_master,
        'to': new_master,
        'initiated_by': socket.getfqdn(),
        'initiated_at': get_absolute_now().isoformat(),
    }
    try:
        client.create(path, json.dumps(switchover).encode('utf-8'))
    except NodeExistsError:
        raise MySyncError('another switchover in progress')

    last_path = _mk_path(cid, 'last_switch')
    last_rejected_path = _mk_path(cid, 'last_rejected_switch')
    init_at = dateutil.parser.parse(switchover['initiated_at'])
    init_by = switchover['initiated_by']
    deadline = time.time() + timeout
    while time.time() < deadline:
        last_switchover = _get_last_switchover(client, last_path, last_rejected_path, init_at, init_by)
        if last_switchover:
            if last_switchover['result']:
                return
            else:
                err = last_switchover.get('error', '<unknown>')
                raise MySyncError('Switchover {}=>{} failed! Error: {}'.format(old_master, new_master, err))
        time.sleep(10)

    raise MySyncError('Switchover {}=>{} timed out'.format(old_master, new_master))


def _get_last_switchover(
    client, last_path: str, last_rejected_path: str, init_at: datetime.datetime, init_by: str
) -> Optional[Dict[str, Any]]:
    last_switchover = _get_switchover(client, last_path, init_at, init_by)
    last_rejected_switchover = _get_switchover(client, last_rejected_path, init_at, init_by)

    return last_switchover or last_rejected_switchover


def _get_switchover(client, path: str, init_at: datetime.datetime, init_by: str) -> Optional[Dict[str, Any]]:
    try:
        switchover = json.loads(client.get(path)[0].decode('utf-8'))
    except NoNodeError:
        return None

    if switchover is None or switchover.get('result') is None:
        return None

    initiated_at = dateutil.parser.parse(switchover['initiated_at'])
    initiated_by = switchover['initiated_by']

    if initiated_at != init_at or initiated_by != init_by:
        return None

    result = switchover.get('result').get('ok')

    return {
        'result': result,
        'initiated_by': initiated_by,
        'initiated_at': initiated_at,
    }


@tracing.trace('MySync _StartMaintenance')
def _start_maintenance(client, cid, timeout=180):
    tracing.set_tag('cluster.id', cid)
    path = _mk_path(cid, 'maintenance')
    maintenance = {
        'initiated_by': socket.getfqdn(),
        'initiated_at': get_absolute_now().isoformat(),
    }
    try:
        client.create(path, json.dumps(maintenance).encode('utf-8'))
    except NodeExistsError:
        pass
    deadline = time.time() + timeout
    while time.time() < deadline:
        maintenance = json.loads(client.get(path)[0].decode('utf-8'))
        if maintenance.get('mysync_paused'):
            return
        time.sleep(10)
    raise MySyncError('Maintenance mode start timed out')


@tracing.trace('MySync _StopMaintenance')
def _stop_maintenance(client, cid, timeout=180):
    tracing.set_tag('cluster.id', cid)
    path = _mk_path(cid, 'maintenance')
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            maintenance = json.loads(client.get(path)[0].decode('utf-8'))
        except NoNodeError:
            return
        if not maintenance.get('should_leave'):
            maintenance['should_leave'] = True
            client.set(path, json.dumps(maintenance).encode('utf-8'))
        time.sleep(10)
    raise MySyncError('Maintenance mode stop timed out')


@tracing.trace('MySync _WaitActive')
def _wait_active(client, cid, host, timeout=180):
    deadline = time.time() + timeout
    while time.time() < deadline:
        active = _get_active_nodes(client, cid)
        if host in active:
            return
        time.sleep(10)
    raise MySyncError('host {} is not active'.format(host))


@tracing.trace('MySync _WaitOtherActive')
def _wait_other_active(client, cid, host, timeout=180):
    deadline = time.time() + timeout
    while time.time() < deadline:
        active = _get_active_nodes(client, cid)
        if len([h for h in active if h != host]) > 0:
            return
        time.sleep(10)
    raise MySyncError('no active hosts besides {}'.format(host))


class MySync(BaseProvider):
    """
    MySync provider
    """

    @KAZOO_RETRIES
    @tracing.trace('MySync Ensure Replica')
    def _ensure_replica(self, zk_hosts, cid, host, timeout=180):
        """
        Make sure that host is replica
        """
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)
        tracing.set_tag('cluster.host.fqdn', host)

        with KazooContext(zk_hosts) as client:
            if _get_master(client, cid) == host:
                _switchover(client, cid, host, '', timeout)

    def ensure_replica(self, zk_hosts, cid, host, timeout=180):
        """
        Make sure that host is replica
        """
        self.add_change(Change(f'mysync.switchover.from.{host}', 'initiated', rollback=Change.noop_rollback))
        self._ensure_replica(zk_hosts, cid, host, timeout)

    @KAZOO_RETRIES
    @tracing.trace('MySync Ensure Master')
    def _ensure_master(self, zk_hosts, cid, host, timeout=180):
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)
        tracing.set_tag('cluster.host.fqdn', host)

        with KazooContext(zk_hosts) as client:
            if _get_master(client, cid) != host:
                _switchover(client, cid, '', host, timeout)

    def ensure_master(self, zk_hosts, cid, host='', timeout=180):
        """
        Make sure that host is master
        """
        self.add_change(Change(f'mysync.switchover.to.{host}', 'initiated', rollback=Change.noop_rollback))
        self._ensure_master(zk_hosts, cid, host, timeout)

    @KAZOO_RETRIES
    @tracing.trace('MySync Get Master')
    def _get_master(self, zk_hosts, cid):  # pylint: disable=no-self-use
        """
        Return master fqdn
        """
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)

        with KazooContext(zk_hosts) as client:
            return _get_master(client, cid)

    def get_master(self, zk_hosts, cid):  # pylint: disable=no-self-use
        """
        Return master fqdn
        """
        return self._get_master(zk_hosts, cid)

    @KAZOO_RETRIES
    @tracing.trace('MySync Get HA Nodes')
    def _get_ha_nodes(self, zk_hosts, cid):  # pylint: disable=no-self-use
        """
        Return list of HA nodes
        """
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)

        with KazooContext(zk_hosts) as client:
            return _get_ha_nodes(client, cid)

    def get_ha_nodes(self, zk_hosts, cid):  # pylint: disable=no-self-use
        """
        Return list of HA nodes
        """
        return self._get_ha_nodes(zk_hosts, cid)

    @KAZOO_RETRIES
    @tracing.trace('MySync Absent')
    def _absent(self, zk_hosts, cid):
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)

        with KazooContext(zk_hosts) as client:
            client.delete(_mk_path(cid), recursive=True)

    def absent(self, zk_hosts, cid):
        """
        Drop mysync data from zk
        """
        self.add_change(Change(f'mysync.data.for.{cid}', 'removed'))
        self._absent(zk_hosts, cid)

    @KAZOO_RETRIES
    @tracing.trace('MySync Start Maintenance')
    def _start_maintenance(self, zk_hosts, cid):
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)

        self.logger.info('Starting maintenance')
        with KazooContext(zk_hosts) as client:
            _start_maintenance(client, cid)

    def start_maintenance(self, zk_hosts, cid):
        """
        Start maintenance mode
        """
        self.add_change(
            Change(
                f'mysync.maintenance.{cid}',
                'started',
                rollback=lambda task, safe_revision: self._stop_maintenance(zk_hosts, cid),
                critical=True,
            )
        )
        self._start_maintenance(zk_hosts, cid)

    @KAZOO_RETRIES
    @tracing.trace('MySync Stop Maintenance')
    def _stop_maintenance(self, zk_hosts, cid):
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)

        self.logger.info('Stopping maintenance')
        with KazooContext(zk_hosts) as client:
            _stop_maintenance(client, cid)

    def stop_maintenance(self, zk_hosts, cid):
        """
        Stop maintenance mode
        """
        self.add_change(Change(f'mysync.maintenance.{cid}', 'stopped', rollback=Change.noop_rollback))
        self._stop_maintenance(zk_hosts, cid)

    @KAZOO_RETRIES
    @tracing.trace('MySync HA Host Absent')
    def _ha_host_absent(self, zk_hosts, cid, host):
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)
        tracing.set_tag('cluster.host.fqdn', host)

        path = os.path.join('/mysql', cid, 'ha_nodes', host)
        with KazooContext(zk_hosts) as client:
            if client.exists(path):
                client.delete(path, recursive=True)

    def ha_host_absent(self, zk_hosts, cid, host):
        """
        Make sure that host is not in HA group
        """
        self.add_change(
            Change(
                f'mysync.ha-node.{host}',
                'removed',
                rollback=Change.noop_rollback,
            )
        )
        self._ha_host_absent(zk_hosts, cid, host)

    @KAZOO_RETRIES
    @tracing.trace('MySync Cascade Host Absent')
    def _cascade_host_absent(self, zk_hosts, cid, host):
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)
        tracing.set_tag('cluster.host.fqdn', host)

        path = os.path.join('/mysql', cid, 'cascade_nodes', host)
        with KazooContext(zk_hosts) as client:
            if client.exists(path):
                client.delete(path, recursive=True)

    def cascade_host_absent(self, zk_hosts, cid, host):
        """
        Make sure that host is not in cascade group
        """
        self.add_change(
            Change(
                f'mysync.cascade-node.{host}',
                'removed',
                rollback=Change.noop_rollback,
            )
        )
        self._cascade_host_absent(zk_hosts, cid, host)

    @tracing.trace('MySync Wait Host Active')
    def wait_host_active(self, zk_hosts, cid, host):
        """
        Wait for host to become active (ready for failover)
        """
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)
        tracing.set_tag('cluster.host.fqdn', host)

        with KazooContext(zk_hosts) as client:
            _wait_active(client, cid, host)

    @tracing.trace('MySync Other Hosts Active')
    def wait_other_hosts_active(self, zk_hosts, cid, host):
        """
        Wait for at least one other host to become active (ready for failover)
        """
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('cluster.id', cid)

        with KazooContext(zk_hosts) as client:
            _wait_other_active(client, cid, host)
