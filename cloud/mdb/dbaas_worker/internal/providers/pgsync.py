"""
Pgsync interaction module
"""

import json
import os
import time

from dbaas_common import retry, tracing
from kazoo.exceptions import NoNodeError

from ..exceptions import ExposedException, UserExposedException
from .common import BaseProvider, Change
from .zookeeper import KAZOO_RETRIES, KazooContext


class PgSyncError(ExposedException):
    """
    Base pgsync interaction error
    """


@retry.on_exception(PgSyncError, factor=2, max_wait=60, max_tries=5)
@KAZOO_RETRIES
@tracing.trace('PGSync _GetMaster')
def _get_master(client, prefix, logger):
    """
    Get current leader lock holder
    """
    tracing.set_tag('zookeeper.prefix', prefix)

    lock = client.Lock(os.path.join(prefix, 'leader'))
    contenders = lock.contenders()
    if contenders and contenders[0]:
        tracing.set_tag('pgsync.master', contenders[0])
        return contenders[0]

    # If there are no one holds the leader lock, we need to check
    # special master node in maintenance which contains master fqdn.
    logger.info('No one holds the leader lock for %s. Checking for maintenance master', prefix)
    try:
        maintenance_master_path = os.path.join(prefix, 'maintenance/master')
        last_master_before_maintenance = client.get(maintenance_master_path)
        if last_master_before_maintenance and last_master_before_maintenance[0]:
            value = last_master_before_maintenance[0].decode('utf-8')
            if '.' in value:
                return value
            logger.info('Got %s on %s. Removing %s', value, maintenance_master_path, maintenance_master_path)
            client.delete(maintenance_master_path)
    except NoNodeError:
        pass

    raise PgSyncError('No one holds the leader lock')


@tracing.trace('PGSync Switchover')
def _switchover(client, prefix, master, new_master=None, timeout=1800):
    """
    Initiate switchover and wait for completion
    """
    tracing.set_tag('zookeeper.prefix', prefix)

    timeline = int(client.get(os.path.join(prefix, 'timeline'))[0].decode('utf-8'))
    paths = ['master', 'state']
    for path in paths:
        client.ensure_path(os.path.join(prefix, 'switchover', path))
    client.set(
        os.path.join(prefix, 'switchover/master'),
        json.dumps(
            {
                'hostname': master,
                'destination': new_master,
                'timeline': timeline,
            }
        ).encode('utf-8'),
    )
    client.set(
        os.path.join(prefix, 'switchover/state'),
        'scheduled'.encode('utf-8'),
    )
    deadline = time.time() + timeout
    while time.time() < deadline:
        state = client.get(os.path.join(prefix, 'switchover/state'))[0].decode('utf-8')
        if state == 'finished':
            return
        if state == 'failed':
            raise PgSyncError('Switchover from {fqdn} failed'.format(fqdn=master))
        time.sleep(10)

    raise PgSyncError('Switchover from {fqdn} timed out'.format(fqdn=master))


def pgsync_cluster_prefix(cid: str) -> str:
    """
    Return pgsync prefix
    """
    return '/pgsync/{cid}'.format(cid=cid)


class PgSync(BaseProvider):
    """
    PgSync provider
    """

    @KAZOO_RETRIES
    @tracing.trace('PGSync Ensure Replica')
    def _ensure_replica(self, host, zk_hosts, prefix):
        tracing.set_tag('cluster.host.fqdn', host)
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('zookeeper.prefix', prefix)

        with KazooContext(zk_hosts) as client:
            try:
                master = _get_master(client, prefix, self.logger)
            except PgSyncError:
                master = None
            if master == host or not master:
                _switchover(client, prefix, master)

    def ensure_replica(self, host, zk_hosts, prefix):
        """
        Make sure that host is replica
        """
        self.add_change(Change(f'pgsync.switchover.from.{host}', 'initiated', rollback=Change.noop_rollback))
        self._ensure_replica(host, zk_hosts, prefix)

    @KAZOO_RETRIES
    @tracing.trace('PGSync Ensure Master')
    def _ensure_master(self, host, zk_hosts, prefix):
        tracing.set_tag('cluster.host.fqdn', host)
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('zookeeper.prefix', prefix)

        with KazooContext(zk_hosts) as client:
            try:
                master = _get_master(client, prefix, self.logger)
            except PgSyncError:
                raise UserExposedException(
                    "Choosing a new master is not possible when the old one is dead. Please use automatic selection."
                )
            if master != host:
                _switchover(client, prefix, master, host)

    def ensure_master(self, host, zk_hosts, prefix):
        """
        Make sure that host is master
        """
        self.add_change(Change(f'pgsync.switchover.to.{host}', 'initiated', rollback=Change.noop_rollback))
        self._ensure_master(host, zk_hosts, prefix)

    @KAZOO_RETRIES
    @tracing.trace('PGSync Get Master')
    def _get_master(self, zk_hosts, prefix):  # pylint: disable=no-self-use
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('zookeeper.prefix', prefix)

        with KazooContext(zk_hosts) as client:
            master = _get_master(client, prefix, self.logger)
            return master

    @tracing.trace('PGSync Get Master')
    def get_master(self, zk_hosts, prefix):  # pylint: disable=no-self-use
        """
        Return master fqdn
        """
        return self._get_master(zk_hosts, prefix)

    @KAZOO_RETRIES
    @tracing.trace('PGSync Start Maintenance')
    def _start_maintenance(self, zk_hosts, prefix):
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('zookeeper.prefix', prefix)

        self.logger.info('Starting maintenance')
        with KazooContext(zk_hosts) as client:
            path = os.path.join(prefix, 'maintenance')
            client.ensure_path(path)
            client.set(path, 'enable'.encode('utf-8'))

    def start_maintenance(self, zk_hosts, prefix):
        """
        Start maintenance mode
        """
        self.add_change(
            Change(
                f'pgsync.maintenance.{prefix}',
                'started',
                rollback=lambda task, safe_revision: self._stop_maintenance(zk_hosts, prefix),
                critical=True,
            )
        )
        self._start_maintenance(zk_hosts, prefix)

    @KAZOO_RETRIES
    @tracing.trace('PGSync Stop Maintenance')
    def _stop_maintenance(self, zk_hosts, prefix):
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('zookeeper.prefix', prefix)

        self.logger.info('Stopping maintenance')
        with KazooContext(zk_hosts) as client:
            path = os.path.join(prefix, 'maintenance')
            client.ensure_path(path)
            client.set(path, 'disable'.encode('utf-8'))

    def stop_maintenance(self, zk_hosts, prefix):
        """
        Stop maintenance mode
        """
        self.add_change(Change(f'pgsync.maintenance.{prefix}', 'stopped', rollback=Change.noop_rollback))
        self._stop_maintenance(zk_hosts, prefix)
