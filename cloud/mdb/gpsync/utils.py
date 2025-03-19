"""
Utility functions for various tasks like switchover, ZK init, etc
"""
# encoding: utf-8

from __future__ import absolute_import, print_function, unicode_literals

import time
import json
import logging
import copy
from operator import itemgetter

from . import zk, read_config
from .exceptions import SwitchoverException


class Switchover(object):
    """
    1. Collect coordinates of the systems being switched over
    2. Check if there is already a switchover in progress. If there is,
       signal its state and coordinates in log.
    3. Initiate switchover.
    4. in blocking mode, attach to ZK and wait for changes in state (either fail
    or success.)
    5. If not in progress, initate. If nonblocking mode is enabled, return.
    """

    def __init__(
        self,
        conf=None,
        master=None,
        syncrep=None,
        timeline=None,
        new_master=None,
        timeout=60,
        config_path='/etc/gpsync.conf',
    ):
        """
        Define configuration of the switchover: if None, then autodetect from
        ZK.
        """
        self.timeout = timeout
        self._log = logging.getLogger('switchover')
        # Might be useful to read from default config in case the class is being
        # called from outside of the CLI utility.
        if conf is None:
            conf = read_config({'config_file': config_path})
        self._conf = conf
        self._zk = zk.Zookeeper(config=conf, plugins=None)
        # If master or syncrep or timeline is provided, use them instead.
        # Autodetect (from ZK) if none.
        self._new_master = new_master
        self._plan = self._get_lock_owners(master, syncrep, timeline)

    def is_possible(self):
        """
        Check, whether it's possible to perform switchover now.
        """
        if self.in_progress():
            logging.error('Switchover is already in progress: %s' % self.state())
            return False
        if self._new_master is not None:
            is_alive = zk.is_host_alive(self._zk, self._new_master, self.timeout / 2)
            if not is_alive:
                logging.error('Cannot promote dead host: %s' % self._new_master)
                return False
            is_ha = self._is_ha(self._new_master)
            if not is_ha:
                logging.error('Cannot promote non ha host: %s' % self._new_master)
                return False
        else:
            replics_info = self._zk.get(zk.REPLICS_INFO_PATH, preproc=json.loads)
            if replics_info:
                replics = list(map(itemgetter('client_hostname'), replics_info))
                for replic in replics:
                    if zk.is_host_alive(self._zk, replic, 1) and self._is_ha(replic):
                        # Ok, there is a suitable candidate for switchover
                        return True
            logging.error('Cannot promote because has no suitable replic for switchover.')
            return False
        return True

    def perform(self, min_replicas, block=True, timeout=None):
        """
        Perform the actual switchover.
        """
        min_replicas = min(min_replicas, len(zk.get_alive_hosts(self._zk, 1)) - 1)
        if timeout is None:
            timeout = self.timeout
        self._initiate_switchover(
            master=self._plan['master'], timeline=self._plan['timeline'], new_master=self._new_master
        )
        if not block:
            return True
        limit = timeout
        while self.in_progress():
            self._log.debug('current switchover status: %(progress)s, failover:' ' %(failover)s', self.state())
            if limit <= 0:
                raise SwitchoverException('timeout exceeded, current status: %s' % self.in_progress())
            time.sleep(1)
            limit -= 1
        self._wait_for_master()
        state = self.state()
        self._log.debug('full state: %s', state)
        self._wait_for_replicas(min_replicas)
        # We delete all zk states after switchover complete
        self._log.info('switchover finished, zk status "%(progress)s"', state)
        result = True if state['progress'] is None else False
        return result

    def in_progress(self, master=None, timeline=None):
        """
        Return True if the cluster is currently in the process of switching
        over.
        Optionally check for specific hostname being currently the master
        and having a particular timeline.
        """
        state = self.state()
        # Check if cluster is in process of switching over
        if state['progress'] in ('failed', None):
            return False
        # The constraint, if specified, must match for this function to return
        # True (actual state)
        conditions = [
            master is None or master == state['info'].get('master'),
            timeline is None or timeline == state['info'].get('timeline'),
        ]
        if all(conditions):
            return state['progress']
        return False

    def state(self):
        """
        Current cluster state.
        """
        return {
            'progress': self._zk.noexcept_get(zk.SWITCHOVER_STATE_PATH),
            'info': self._zk.noexcept_get(zk.SWITCHOVER_MASTER_PATH, preproc=json.loads) or {},
            'failover': self._zk.noexcept_get(zk.FAILOVER_INFO_PATH),
            'replicas': self._zk.noexcept_get(zk.REPLICS_INFO_PATH, preproc=json.loads) or {},
        }

    def plan(self):
        """
        Get switchover plan
        """
        return copy.deepcopy(self._plan)

    def _get_lock_owners(self, master=None, syncrep=None, timeline=None):
        """
        Get leader and syncreplica lock owners, and timeline.
        """
        owners = {
            'master': master or self._zk.get_current_lock_holder(zk.MASTER_LOCK_PATH),
            'sync_replica': syncrep or self._zk.get_current_lock_holder(zk.SYNC_REPLICA_LOCK_PATH),
            'timeline': timeline or self._zk.noexcept_get(zk.TIMELINE_INFO_PATH, preproc=int),
        }
        self._log.debug('lock holders: %s', owners)
        return owners

    def reset(self, force=False):
        """
        Reset state and hostname-timeline
        """
        self._log.info('resetting ZK switchover nodes')
        if not force and self.in_progress():
            raise SwitchoverException('attempted to reset state while' ' switchover is in progress')
        self._lock(zk.SWITCHOVER_LOCK_PATH)
        if not self._zk.noexcept_write(zk.SWITCHOVER_MASTER_PATH, '{}', need_lock=False):
            raise SwitchoverException('unable to reset node %s' % zk.SWITCHOVER_MASTER_PATH)
        return True

    def _is_ha(self, hostname):
        """
        Checks whether given host is ha replica.
        """
        ha_path = '{member_path}/{host}/ha'.format(
            member_path=zk.MEMBERS_PATH,
            host=hostname,
        )
        return self._zk.exists_path(ha_path)

    def _lock(self, node):
        """
        Lock switchover structure in ZK
        """
        if not self._zk.ensure_path(node):
            raise SwitchoverException('unable to create switchover node (%s)' % node)
        if not self._zk.try_acquire_lock(lock_type=node, allow_queue=True, timeout=self.timeout):
            raise SwitchoverException('unable to lock switchover node (%s)' % node)

    def _initiate_switchover(self, master, timeline, new_master):
        """
        Write master coordinates and 'scheduled' into state node to
        initiate switchover.
        1. Lock the hostname-timeline json node.
        2. Set hostname, timeline and destination.
        3. Set state to 'scheduled'
        """
        switchover_task = {
            'hostname': master,
            'timeline': timeline,
            'destination': new_master,
        }
        self._log.info('initiating switchover with %s', switchover_task)
        self._lock(zk.SWITCHOVER_LOCK_PATH)
        self._zk.write(zk.SWITCHOVER_MASTER_PATH, switchover_task, preproc=json.dumps, need_lock=False)
        self._zk.write(zk.SWITCHOVER_STATE_PATH, 'scheduled', need_lock=False)
        self._log.debug('state: %s', self.state())

    def _wait_for_replicas(self, min_replicas, timeout=None):
        """
        Wait for replicas to appear
        """
        if timeout is None:
            timeout = self.timeout
        self._log.debug('waiting for replicas to appear...')
        for _ in range(timeout):
            time.sleep(1)
            replicas = [
                '%s@%s' % (x['client_hostname'], x['master_location'])
                for x in self.state()['replicas']
                if x['state'] == 'streaming'
            ]
            self._log.debug('replicas up: %s', (', '.join(replicas) if replicas else 'none'))
            if len(replicas) >= min_replicas:
                return replicas
        raise SwitchoverException(
            'expected {rep_num} replicas to appear within {s} secs, got {rep_up}'.format(
                rep_num=min_replicas, s=timeout, rep_up=len(self.state()['replicas'])
            )
        )

    def _wait_for_master(self, timeout=None):
        """
        Wait for master to hold the lock
        """
        if timeout is None:
            timeout = self.timeout
        for _ in range(timeout):
            time.sleep(1)
            holder = self._zk.get_current_lock_holder(zk.MASTER_LOCK_PATH)
            if holder is not None:
                self._log.info('master is now %s', holder)
                return holder
            self._log.debug('waiting for new master to acquire lock...')
        raise SwitchoverException('no one took master lock in %i secs' % timeout)
