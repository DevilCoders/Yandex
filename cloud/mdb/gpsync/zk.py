# encoding: utf-8
"""
Zookeeper wrapper module. Zookeeper class defined here.
"""

from __future__ import absolute_import, print_function, unicode_literals

import json
import logging
import os
import traceback
import time

from kazoo.client import KazooClient, KazooState
from kazoo.exceptions import LockTimeout, NoNodeError, KazooException, ConnectionClosedError
from kazoo.handlers.threading import KazooTimeoutError, SequentialThreadingHandler

from . import helpers

MASTER_LOCK_PATH = 'leader'
REMASTER_LOCK_PATH = 'remaster'
SYNC_REPLICA_LOCK_PATH = 'sync_replica'

QUORUM_PATH = 'quorum'
QUORUM_MEMBER_LOCK_PATH = f'{QUORUM_PATH}/members/%s'

REPLICS_INFO_PATH = 'replics_info'
DBID_INFO_PATH = 'dbid_info'
CREATE_DBID_FOR_STANDBY_REQUEST_PATH = 'create_dbid_for_host'
TIMELINE_INFO_PATH = 'timeline'
FAILOVER_INFO_PATH = 'failover_state'
CURRENT_PROMOTING_HOST = 'current_promoting_host'
LAST_FAILOVER_TIME_PATH = 'last_failover_time'
LAST_MASTER_AVAILABILITY_TIME = 'last_master_activity_time'
LAST_SWITCHOVER_TIME_PATH = 'last_switchover_time'
SWITCHOVER_ROOT_PATH = 'switchover'
SWITCHOVER_LOCK_PATH = '%s/lock' % SWITCHOVER_ROOT_PATH
SWITCHOVER_LSN_PATH = f'{SWITCHOVER_ROOT_PATH}/lsn'
# A JSON string with master fqdn and its timeline
SWITCHOVER_MASTER_PATH = '%s/master' % SWITCHOVER_ROOT_PATH
# A simple string with current scheduled switchover state
SWITCHOVER_STATE_PATH = '%s/state' % SWITCHOVER_ROOT_PATH
MAINTENANCE_PATH = 'maintenance'
MAINTENANCE_TIME_PATH = 'maintenance/ts'
HOST_MAINTENANCE_PATH = 'maintenance/%s'
HOST_ALIVE_LOCK_PATH = 'alive/%s'

SINGLE_NODE_PATH = 'is_single_node'

ELECTION_ENTER_LOCK_PATH = 'enter_election'
ELECTION_MANAGER_LOCK_PATH = 'epoch_manager'
ELECTION_WINNER_PATH = 'election_winner'
ELECTION_STATUS_PATH = 'election_status'
ELECTION_VOTE_PATH = 'election_vote/%s'

MEMBERS_PATH = 'all_hosts'
SIMPLE_REMASTER_TRY_PATH = f'{MEMBERS_PATH}/%s/tried_remaster'
HOST_PRIO_PATH = f'{MEMBERS_PATH}/%s/prio'


def get_host_alive_lock_path(hostname=None):
    return _get_host_path(HOST_ALIVE_LOCK_PATH, hostname)


def get_host_maintenance_path(hostname=None):
    return _get_host_path(HOST_MAINTENANCE_PATH, hostname)


def get_host_quorum_path(hostname=None):
    return _get_host_path(QUORUM_MEMBER_LOCK_PATH, hostname)


def get_host_prio_path(hostname=None):
    return _get_host_path(HOST_PRIO_PATH, hostname)


def get_simple_remaster_try_path(hostname=None):
    return _get_host_path(SIMPLE_REMASTER_TRY_PATH, hostname)


def _get_host_path(path, hostname):
    if hostname is None:
        hostname = helpers.get_hostname()
    return path % hostname


def is_host_alive(zk, hostname, timeout=0.0):
    alive_path = get_host_alive_lock_path(hostname)
    return helpers.await_for(
        lambda: zk.get_current_lock_holder(alive_path) is not None, timeout, f'{hostname} is alive'
    )


def get_election_vote_path(hostname=None):
    if hostname is None:
        hostname = helpers.get_hostname()
    return ELECTION_VOTE_PATH % hostname


def get_ha_hosts(zk):
    all_hosts = zk.get_children(MEMBERS_PATH)
    if all_hosts is None:
        logging.error('Failed to get HA host list from ZK')
        return None
    ha_hosts = []
    for host in all_hosts:
        path = f"{MEMBERS_PATH}/{host}/ha"
        if zk.exists_path(path):
            ha_hosts.append(host)
    logging.debug(f"HA hosts are: {ha_hosts}")
    return ha_hosts


def _is_host_in_sync_quorum(zk, hostname):
    host_quorum_path = get_host_quorum_path(hostname)
    return zk.get_current_lock_holder(host_quorum_path) is not None


def get_sync_quorum_hosts(zk):
    all_hosts = zk.get_children(MEMBERS_PATH)
    if all_hosts is None:
        logging.error('Failed to get HA host list from ZK')
        return []
    return [host for host in all_hosts if _is_host_in_sync_quorum(zk, host)]


def get_alive_hosts(zk, timeout=1):
    ha_hosts = get_ha_hosts(zk)
    if ha_hosts is None:
        return []
    alive_hosts = [host for host in ha_hosts if is_host_alive(zk, host, timeout)]
    return alive_hosts


class ZookeeperException(Exception):
    """Exception for wrapping all zookeeper connector inner exceptions"""


class Zookeeper(object):
    """
    Zookeeper class
    """

    def __init__(self, config, plugins):
        self._plugins = plugins
        self._zk_hosts = config.get('global', 'zk_hosts')
        self._timeout = config.getfloat('global', 'iteration_timeout')
        try:
            self._locks = {}
            prefix = config.get('global', 'zk_lockpath_prefix')
            self._path_prefix = prefix if prefix is not None else helpers.get_lockpath_prefix()
            self._lockpath = self._path_prefix + MASTER_LOCK_PATH

            self._create_kazoo_client()
            event = self._zk.start_async()
            event.wait(self._timeout)
            if not self._zk.connected:
                raise Exception('Could not connect to ZK.')
            self._zk.add_listener(self._listener)
            self._init_lock(MASTER_LOCK_PATH)
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())

    def __del__(self):
        self._zk.remove_listener(self._listener)
        self._zk.stop()

    def _create_kazoo_client(self):
        conn_retry_options = {'max_tries': 10, 'delay': 0.5, 'backoff': 1.5, 'max_delay': 60}
        command_retry_options = {'max_tries': 0, 'delay': 0, 'backoff': 1, 'max_delay': 1}

        self._zk = KazooClient(
            hosts=self._zk_hosts,
            handler=SequentialThreadingHandler(),
            timeout=self._timeout,
            connection_retry=conn_retry_options,
            command_retry=command_retry_options,
        )

    def _listener(self, state):
        if state == KazooState.LOST:
            # In the event that a LOST state occurs, its certain that the lock and/or the lease has been lost.
            logging.error("Connection to ZK lost, clean all locks")
            self._locks = {}
            self._plugins.run('on_lost')
        elif state == KazooState.SUSPENDED:
            logging.warning("Being disconnected from ZK.")
            self._plugins.run('on_suspend')
        elif state == KazooState.CONNECTED:
            logging.info("Reconnected to ZK.")
            self._plugins.run('on_connect')

    def _wait(self, event):
        event.wait(self._timeout)

    def _get(self, path):
        event = self._zk.get_async(path)
        self._wait(event)
        return event.get_nowait()

    #
    # We assume data is already converted to text.
    #
    def _write(self, path, data, need_lock=True):
        if need_lock and self.get_current_lock_holder() != helpers.get_hostname():
            return False
        event = self._zk.exists_async(path)
        self._wait(event)
        if event.get_nowait():  # Node exists
            event = self._zk.set_async(path, data.encode())
        else:
            event = self._zk.create_async(path, value=data.encode())
        self._wait(event)
        if event.exception:
            logging.error('Failed to write to node: %s.' % path)
            logging.error(event.exception)
        return not event.exception

    def _init_lock(self, name=MASTER_LOCK_PATH):
        path = self._path_prefix + name
        self._locks[name] = self._zk.Lock(path, helpers.get_hostname())

    def _acquire_lock(self, name, allow_queue, timeout):
        if timeout is None:
            timeout = self._timeout
        if self._zk.state != KazooState.CONNECTED:
            logging.warning('Not able to acquire %s ' % name + 'lock without alive connection.')
            return False
        if name in self._locks:
            lock = self._locks[name]
        else:
            logging.debug('No lock instance for %s. Creating one.', name)
            self._init_lock(name)
            lock = self._locks[name]
        contenders = lock.contenders()
        if len(contenders) != 0:
            if contenders[0] == helpers.get_hostname():
                logging.debug('We already hold the %s lock.', name)
                return True
            if not allow_queue:
                logging.warning('%s lock is already taken by %s.', name[0].upper() + name[1:], contenders[0])
                return False
        try:
            return lock.acquire(blocking=True, timeout=timeout)
        except LockTimeout:
            logging.warning('Unable to obtain lock %s within timeout (%s s)', name, timeout)
            return False
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return False

    def _release_lock(self, name):
        if name in self._locks:
            return self._locks[name].release()

    def is_alive(self):
        """
        Return True if we are connected to zk
        """
        if self._zk.state == KazooState.CONNECTED:
            return True
        return False

    def reconnect(self):
        """
        Reconnect to zk
        """
        try:
            for lock in self._locks.items():
                if lock[1]:
                    lock[1].release()
        except (KazooException, KazooTimeoutError):
            pass

        try:
            self._locks = {}
            self._zk.stop()
            self._zk.close()
            self._create_kazoo_client()
            event = self._zk.start_async()
            event.wait(self._timeout)
            if not self._zk.connected:
                return False

            self._zk.add_listener(self._listener)
            self._init_lock(MASTER_LOCK_PATH)
            return self.is_alive()
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return False

    def get(self, key, preproc=None):
        """
        Get key value from zk
        """
        path = self._path_prefix + key
        try:
            res = self._get(path)
        except NoNodeError:
            return None
        except (KazooException, KazooTimeoutError) as exception:
            raise ZookeeperException(exception)
        value = res[0].decode('utf-8')
        if preproc:
            return preproc(value)
        else:
            return value

    @helpers.return_none_on_error
    def noexcept_get(self, key, preproc=None):
        """
        Get key value from zk, without ZK exception forwarding
        """
        return self.get(key, preproc)

    @helpers.return_none_on_error
    def get_mtime(self, key):
        """
        Returns modification time of ZK node
        """
        return getattr(self._get_meta(key), 'last_modified', None)

    def _get_meta(self, key):
        """
        Get metadata from key.
        returns kazoo.protocol.states.ZnodeStat
        """
        path = self._path_prefix + key
        try:
            (_, meta) = self._get(path)
        except NoNodeError:
            return None
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return None
        else:
            return meta

    def ensure_path(self, path):
        """
        Check that path exists and create if not
        """
        if not path.startswith(self._path_prefix):
            path = os.path.join(self._path_prefix, path)
        event = self._zk.ensure_path_async(path)
        try:
            return event.get(timeout=self._timeout)
        except (KazooException, KazooTimeoutError):
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return None

    def exists_path(self, path):
        if not path.startswith(self._path_prefix):
            path = os.path.join(self._path_prefix, path)
        event = self._zk.exists_async(path)
        try:
            self._wait(event)
        except (KazooException, KazooTimeoutError):
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return False
        else:
            return bool(event.get_nowait())

    def get_children(self, path):
        """
        Get children nodes of path
        """
        try:
            if not path.startswith(self._path_prefix):
                path = os.path.join(self._path_prefix, path)
            event = self._zk.get_children_async(path)
            self._wait(event)
            return event.get_nowait()
        except NoNodeError:
            for line in traceback.format_exc().split('\n'):
                logging.debug(line.rstrip())
            return None
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return None

    def get_state(self):
        """
        Get current zk state (if possible)
        """
        data = {'alive': self.is_alive()}
        if not data['alive']:
            raise ZookeeperException("Zookeeper connection is unavailable now")
        data['replics_info'] = self.get(REPLICS_INFO_PATH, preproc=json.loads)
        data['master_dbids'] = self.get(DBID_INFO_PATH, preproc=json.loads)
        data['last_failover_time'] = self.get(LAST_FAILOVER_TIME_PATH, preproc=float)
        data['failover_state'] = self.get(FAILOVER_INFO_PATH)
        data['current_promoting_host'] = self.get(CURRENT_PROMOTING_HOST)
        data['lock_version'] = self.get_current_lock_version()
        data['lock_holder'] = self.get_current_lock_holder()
        data['single_node'] = self.exists_path(SINGLE_NODE_PATH)
        data['timeline'] = self.get(TIMELINE_INFO_PATH, preproc=int)
        data['switchover'] = self.get(SWITCHOVER_MASTER_PATH, preproc=json.loads)
        data['maintenance'] = {'status': self.get(MAINTENANCE_PATH), 'ts': self.get(MAINTENANCE_TIME_PATH)}

        data['alive'] = self.is_alive()
        if not data['alive']:
            raise ZookeeperException("Zookeeper connection is unavailable now")
        return data

    def _preproc_write(self, key, data, preproc):
        path = self._path_prefix + key
        if preproc:
            sdata = preproc(data)
        else:
            sdata = str(data)
        return path, sdata

    def write(self, key, data, preproc=None, need_lock=True):
        """
        Write value to key in zk
        """
        path, sdata = self._preproc_write(key, data, preproc)
        try:
            return self._write(path, sdata, need_lock=need_lock)
        except (KazooException, KazooTimeoutError) as exception:
            raise ZookeeperException(exception)

    def noexcept_write(self, key, data, preproc=None, need_lock=True):
        """
        Write value to key in zk without zk exceptions forwarding
        """
        path, sdata = self._preproc_write(key, data, preproc)
        try:
            return self._write(path, sdata, need_lock=need_lock)
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return False

    def delete(self, key, recursive=False):
        """
        Delete key from zk
        """
        path = self._path_prefix + key
        try:
            self._zk.delete(path, recursive=recursive)
            return True
        except NoNodeError:
            logging.info('No node %s was found in ZK to delete it.' % key)
            return True
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return False

    def get_current_lock_version(self):
        """
        Get current leader lock version
        """
        children = self.get_children(self._lockpath)
        if children and len(children) > 0:
            return min([i.split('__')[-1] for i in children])
        return None

    def get_lock_contenders(self, name=MASTER_LOCK_PATH):
        """
        Get a list of all hostnames that are competing for the lock,
        including the holder.
        """
        try:
            if name not in self._locks:
                self._init_lock(name)
            contenders = self._locks[name].contenders()
            if len(contenders) > 0:
                return contenders
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.debug(line.rstrip())
        return []

    def get_current_lock_holder(self, name=MASTER_LOCK_PATH):
        """
        Get hostname of lock holder
        """
        lock_contenders = self.get_lock_contenders(name)
        if len(lock_contenders) > 0:
            return lock_contenders[0]
        else:
            return None

    def acquire_lock(self, lock_type, allow_queue=False, timeout=None):
        result = self._acquire_lock(lock_type, allow_queue, timeout)
        if not result:
            raise ZookeeperException(f'Failed to acquire lock {lock_type}')
        logging.debug(f'Success acquire lock: {lock_type}')

    def try_acquire_lock(self, lock_type=MASTER_LOCK_PATH, allow_queue=False, timeout=None):
        """
        Acquire lock (leader by default)
        """
        return self._acquire_lock(lock_type, allow_queue, timeout)

    def release_lock(self, lock_type=MASTER_LOCK_PATH, wait=0):
        """
        Release lock (leader by default)
        """
        # If caller decides to rely on kazoo internal API,
        # release the lock and return immediately.
        if not wait:
            return self._release_lock(lock_type)

        # Otherwise, make sure the lock is actually released.
        hostname = helpers.get_hostname()
        for _ in range(wait):
            try:
                self._release_lock(lock_type)
                holder = self.get_current_lock_holder(name=lock_type)
                if holder != hostname:
                    return True
            except ConnectionClosedError:
                # ok, shit happens, now we should reconnect to ensure that we actually released the lock
                self.reconnect()
            logging.warning('Unable to release lock "%s", retrying', lock_type)
            time.sleep(1)
        raise RuntimeError('unable to release lock after %i attempts' % wait)

    def release_if_hold(self, lock_type, wait=0):
        holder = self.get_current_lock_holder(lock_type)
        if holder != helpers.get_hostname():
            return True
        return self.release_lock(lock_type, wait)
