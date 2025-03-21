"""
Main module. Pgsync class defined here.
"""
# encoding: utf-8

from __future__ import absolute_import, print_function, unicode_literals

import atexit
import functools
import json
import logging
import os
import random
import signal
import sys
import time
import traceback

import psycopg2

from . import helpers, sdnotify, zk
from .command_manager import CommandManager
from .failover_election import ElectionError, FailoverElection
from .helpers import IterationTimer, conninfo_to_dict, get_hostname
from .pg import Postgres
from .plugin import PluginRunner, load_plugins
from .replication_manager import QuorumReplicationManager, SingleSyncReplicationManager
from .zk import Zookeeper, ZookeeperException


class Pgsync(object):
    """
    Pgsync class
    """

    DESTRUCTIVE_OPERATIONS = ['rewind']

    def __init__(self, **kwargs):
        logging.debug('Initializing main class.')
        self.config = kwargs.get('config')
        self._cmd_manager = CommandManager(self.config)
        self._should_run = True
        self.is_in_maintenance = False

        random.seed(os.urandom(16))

        plugins = load_plugins(self.config.get('global', 'plugins_path'))

        self.db = Postgres(config=self.config, plugins=PluginRunner(plugins['Postgres']), cmd_manager=self._cmd_manager)
        self.zk = Zookeeper(config=self.config, plugins=PluginRunner(plugins['Zookeeper']))
        self.startup_checks()

        signal.signal(signal.SIGTERM, self._sigterm_handler)

        self.checks = {'remaster': 0, 'failover': 0, 'rewind': 0}
        self._is_single_node = False
        self.notifier = sdnotify.Notifier()

        if self.config.getboolean('global', 'quorum_commit'):
            self._replication_manager = QuorumReplicationManager(
                self.config,
                self.db,
                self.zk,
            )
        else:
            self._replication_manager = SingleSyncReplicationManager(
                self.config,
                self.db,
                self.zk,
            )

    def _sigterm_handler(self, *_):
        self._should_run = False

    def re_init_db(self):
        """
        Reinit db connection
        """
        try:
            if not self.db.is_alive():
                db_state = self.db.get_state()
                logging.error(
                    'Could not get data from PostgreSQL. Seems, '
                    'that it is dead. Getting last role from cached '
                    'file. And trying to reconnect.'
                )
                if db_state.get('prev_state'):
                    self.db.role = db_state['prev_state']['role']
                    self.db.pg_version = db_state['prev_state']['pg_version']
                    self.db.pgdata = db_state['prev_state']['pgdata']
                self.db.reconnect()
        except KeyError:
            logging.error('Could not get data from PostgreSQL and ' 'cache-file. Exiting.')
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            sys.exit(1)
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())

    def re_init_zk(self):
        """
        Reinit zk connection
        """
        try:
            if not self.zk.is_alive():
                logging.warning('Some error with ZK client. ' 'Trying to reconnect.')
                self.zk.reconnect()
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())

    def startup_checks(self):
        """
        Perform some basic checks on startup
        """
        work_dir = self.config.get('global', 'working_dir')
        fname = '%s/.pgsync_rewind_fail.flag' % work_dir

        if os.path.exists(fname):
            logging.error('Rewind fail flag exists. Exiting.')
            sys.exit(1)

        if self.db.is_alive() and not self.zk.is_alive():
            if self.db.role == 'master' and self.db.pgbouncer('status'):
                self.db.pgbouncer('stop')

        if not self.db.is_alive() and self.zk.is_alive():
            if self.zk.get_current_lock_holder() == helpers.get_hostname():
                res = self.zk.release_lock()
                if res:
                    logging.info('Released lock in ZK since postgres is dead.')

        db_state = self.db.get_state()
        if db_state['prev_state'] is not None:
            # Ok, it means that current start is not the first one.
            # In this case we should check that we are able to do pg_rewind.
            if not db_state['alive']:
                self.db.pgdata = db_state['prev_state']['pgdata']
            if not self.db.is_ready_for_pg_rewind():
                sys.exit(0)

        # Abort startup if zk.MEMBERS_PATH is empty
        # (no one is participating in cluster), but
        # timeline indicates a mature (tli>1) and operating database system.
        tli = self.db.get_state().get('timeline', 0)
        if not self._get_zk_members() and tli > 1:
            logging.error(
                'ZK "%s" empty but timeline indicates' ' operating cluster (%i > 1)',
                zk.MEMBERS_PATH,
                tli,
            )
            self.db.pgbouncer('stop')
            sys.exit(1)

        if (
            self.config.getboolean('global', 'quorum_commit')
            and not self.config.getboolean('global', 'use_lwaldump')
            and not self.config.getboolean('replica', 'allow_potential_data_loss')
        ):
            logging.error("Using quorum_commit allow only with use_lwaldump or with allow_potential_data_loss")
            exit(1)

        if (
            self.db.is_alive()
            and not self.db.check_extension_installed('lwaldump')
            and self.config.getboolean('global', 'use_lwaldump')
        ):
            logging.error("lwaldump is not installed")
            exit(1)

    # pylint: disable=W0212
    def stop(self, *_):
        """
        Stop iterations
        """
        logging.info('Stopping')
        atexit._run_exitfuncs()
        os._exit(0)

    def _init_zk(self, my_prio):
        if not self._replication_manager.init_zk():
            return False

        if not self.config.getboolean('global', 'update_prio_in_zk') and helpers.get_hostname() in self.zk.get_children(
            zk.MEMBERS_PATH
        ):
            logging.info("Don't have to write priority to ZK")
            return True

        return self.zk.ensure_path(zk.get_host_prio_path()) and self.zk.noexcept_write(
            zk.get_host_prio_path(), my_prio, need_lock=False
        )

    def start(self):
        """
        Start iterations
        """
        my_prio = self.config.get('global', 'priority')
        self.notifier.ready()
        while True:
            if self._init_zk(my_prio):
                break
            logging.error('Failed to init ZK')
            self.re_init_zk()

        while self._should_run:
            try:
                self.run_iteration(my_prio)
            except Exception:
                for line in traceback.format_exc().split('\n'):
                    logging.error(line.rstrip())
        self.stop()

    def update_maintenance_status(self, role, master_fqdn):
        maintenance_status = self.zk.get(zk.MAINTENANCE_PATH)  # can be None, 'enable', 'disable'
        if role == 'master':
            if maintenance_status == 'enable':
                if self._update_replication_on_maintenance_enter():
                    self.is_in_maintenance = True
            elif maintenance_status is None:
                # maintenance node doesn't exists, we are not in maintenance mode
                self.is_in_maintenance = False
            elif maintenance_status == 'disable':
                # Maintenance node exists with 'disable' value, we are not in maintenance now
                # and should delete this node. We delete it recursively, we don't won't to wait
                # all cluster members to delete each own node, because some of them may be
                # already dead and we can wait it infinitely. Maybe we should wait each member
                # with timeout and then delete recursively (TODO).
                logging.debug('Disabling maintenance mode, deleting maintenance node')
                self.zk.delete(zk.MAINTENANCE_PATH, recursive=True)
                self.is_in_maintenance = False
            return

        if maintenance_status == 'enable':
            self.is_in_maintenance = True
            # Write current ts to zk on maintenance enabled, it's be dropped on disable
            maintenance_ts = self.zk.get(zk.MAINTENANCE_TIME_PATH)
            if maintenance_ts is None:
                self.zk.write(zk.MAINTENANCE_TIME_PATH, time.time(), need_lock=False)
            # Write current master to zk on maintenance enabled, it's be dropped on disable
            current_master = self.zk.get(zk.MAINTENANCE_MASTER_PATH)
            if current_master is None and master_fqdn is not None:
                self.zk.write(zk.MAINTENANCE_MASTER_PATH, master_fqdn, need_lock=False)
        elif maintenance_status is None:
            self.is_in_maintenance = False

    def _update_replication_on_maintenance_enter(self):
        if not self.config.getboolean('master', 'change_replication_type'):
            # Replication type change is restricted, we do nothing here
            return True
        if self.config.getboolean('master', 'sync_replication_in_maintenance'):
            # It is allowed to have sync replication in maintenance here
            return True
        current_replication = self.db.get_replication_state()
        if current_replication[0] == 'async':
            # Ok, it is already async
            return True
        return self._replication_manager.change_replication_to_async()

    def run_iteration(self, my_prio):
        timer = IterationTimer()
        role = self.db.get_role()
        logging.debug('Role: %s', str(role))

        db_state = self.db.get_state()
        self.notifier.notify()
        logging.debug(db_state)
        try:
            zk_state = self.zk.get_state()
            logging.debug(zk_state)
            helpers.write_status_file(db_state, zk_state, self.config.get('global', 'working_dir'))
            self.update_maintenance_status(role, db_state.get('master_fqdn'))
            self._zk_alive_refresh(role, db_state, zk_state)
            if self.is_in_maintenance:
                logging.warning('Cluster in maintenance mode')
                self.zk.reconnect()
                self.zk.write(zk.get_host_maintenance_path(), 'enable')
                return
        except ZookeeperException:
            logging.error("Zookeeper exception while getting ZK state")
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            if role == 'master' and not self.is_in_maintenance and not self._is_single_node:
                logging.error("Upper exception was for master")
                my_hostname = helpers.get_hostname()
                self.resolve_zk_master_lock(my_hostname)
            else:
                self.re_init_zk()
            return
        stream_from = self.config.get('global', 'stream_from')
        if role is None:
            self.dead_iter(db_state, zk_state)
        elif role == 'master':
            if self._is_single_node:
                self.single_node_master_iter(db_state, zk_state)
            else:
                self.master_iter(db_state, zk_state)
        elif role == 'replica':
            if stream_from:
                self.non_ha_replica_iter(db_state, zk_state)
            else:
                self.replica_iter(db_state, zk_state)
        self.re_init_db()
        self.re_init_zk()

        # Dead PostgreSQL probably means
        # that our node is being removed.
        # No point in updating all_hosts
        # in this case
        all_hosts = self.zk.get_children(zk.MEMBERS_PATH)
        prio = self.zk.noexcept_get(zk.get_host_prio_path())
        if role and all_hosts and not prio:
            if not self.zk.noexcept_write(zk.get_host_prio_path(), my_prio, need_lock=False):
                logging.warning('Could not write priority to ZK')

        logging.debug('Finished iteration.')
        timer.sleep(self.config.getfloat('global', 'iteration_timeout'))

    def release_lock_and_return_to_cluster(self):
        my_hostname = helpers.get_hostname()
        self.db.pgbouncer('stop')
        holder = self.zk.get_current_lock_holder()
        if holder == my_hostname:
            self.zk.release_lock()
        elif holder is not None:
            logging.warning('Lock in ZK is being held by %s. We ' 'should return to cluster here.', holder)
            self._return_to_cluster(holder, 'master')

    def single_node_master_iter(self, db_state, zk_state):
        """
        Iteration if local postgresql is single node
        """
        logging.info('Master is in single node state')
        self.zk.try_acquire_lock()

        tli_res = None
        if zk_state['timeline']:
            tli_res = zk_state['timeline'] == db_state['timeline']
        replics_info = db_state.get('replics_info')

        zk_state['replics_info_written'] = None
        if tli_res and replics_info is not None:
            zk_state['replics_info_written'] = self.zk.write(zk.REPLICS_INFO_PATH, replics_info, preproc=json.dumps)
            self.write_host_stat(helpers.get_hostname(), db_state)

        self.zk.write(zk.TIMELINE_INFO_PATH, db_state['timeline'])

        if not self.db.pgbouncer('status'):
            logging.debug('Here we should open for load.')
            self.db.pgbouncer('start')

        self.db.ensure_archiving_wal()

        # Enable async replication
        current_replication = self.db.get_replication_state()
        if current_replication[0] != 'async':
            self._replication_manager.change_replication_to_async()

    def master_iter(self, db_state, zk_state):
        """
        Iteration if local postgresql is master
        """
        my_hostname = helpers.get_hostname()
        try:
            stream_from = self.config.get('global', 'stream_from')
            last_op = self.zk.get('%s/%s/op' % (zk.MEMBERS_PATH, my_hostname))
            # If we were promoting or rewinding
            # and failed we should not acquire lock
            if self.is_op_destructive(last_op):
                logging.warning('Could not acquire lock ' 'due to destructive ' 'operation fail: %s', last_op)
                return self.release_lock_and_return_to_cluster()
            if stream_from:
                logging.warning('Host not in HA group ' 'We should return to stream_from.')
                return self.release_lock_and_return_to_cluster()

            current_promoting_host = zk_state.get('current_promoting_host')
            if current_promoting_host and current_promoting_host != helpers.get_hostname():
                logging.warning('Host %s was promoted. We should not be master', zk_state['current_promoting_host'])
                self.resolve_zk_master_lock(my_hostname)
                return None

            if not self.zk.try_acquire_lock():
                self.resolve_zk_master_lock(my_hostname)
                return None
            self.zk.write(zk.LAST_MASTER_AVAILABILITY_TIME, time.time())

            self._reset_simple_remaster_try()

            self.checks['remaster'] = 0

            tli_res = None
            if zk_state['timeline']:
                tli_res = zk_state['timeline'] == db_state['timeline']
            replics_info = db_state.get('replics_info')
            self._fix_hostname(replics_info)

            zk_state['replics_info_written'] = None
            if tli_res and replics_info is not None:
                zk_state['replics_info_written'] = self.zk.write(zk.REPLICS_INFO_PATH, replics_info, preproc=json.dumps)
                self.write_host_stat(my_hostname, db_state)

            # Make sure local timeline corresponds to that of the cluster.
            if not self._verify_timeline(db_state, zk_state):
                return None

            # Check for unfinished failover and if self is last promoted host
            # In this case self is fully operational master, need to reset
            # failover state in ZK. Otherwise need to try return to cluster as replica
            if zk_state['failover_state'] in ('promoting', 'checkpointing'):
                if zk_state['current_promoting_host'] == helpers.get_hostname():
                    self.zk.write(zk.FAILOVER_INFO_PATH, 'finished')
                    self.zk.delete(zk.CURRENT_PROMOTING_HOST)
                    logging.info('Resetting failover info (was "%s", now "finished")', zk_state['failover_state'])
                    return None  # so zk_state will be updated in the next iter
                else:
                    logging.info(
                        'Failover state was "%s" and last promoted host was "%s"',
                        zk_state['failover_state'],
                        zk_state['current_promoting_host'],
                    )
                    return self.release_lock_and_return_to_cluster()

            self._drop_stale_switchover(db_state)

            if not self.db.pgbouncer('status'):
                logging.debug('Here we should open for load.')
                self.db.pgbouncer('start')

            # Ensure that wal archiving is enabled. It can be disabled earlier due to
            # some zk connectivity issues.
            self.db.ensure_archiving_wal()

            # Check if replication type (sync/normal) change is needed.
            ha_replics = self._get_ha_replics()
            if ha_replics is None:
                return None
            logging.debug('Checking if changing replication type is needed.')
            change_replication = self.config.getboolean('master', 'change_replication_type')
            if change_replication:
                self._replication_manager.update_replication_type(db_state, ha_replics)

            # Check if scheduled switchover conditions exists
            # and local cluster state can handle switchover.
            if not self._check_master_switchover(db_state, zk_state):
                return None

            # Perform switchover: shutdown user service,
            # release lock, write state.
            if not self._do_master_switchover(db_state):
                return None

            # Ensure that new master will appear in time,
            # and transition current instance to replica.
            # Rollback state if this does not happen.
            return self._transition_master_switchover()

        except ZookeeperException:
            if not self.zk.try_acquire_lock():
                logging.error("Zookeeper error during master iteration:")
                self.resolve_zk_master_lock(my_hostname)
                return None
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return None

    def resolve_zk_master_lock(self, my_hostname):
        holder = self.zk.get_current_lock_holder()
        if holder is None:
            if self._replication_manager.should_close():
                self.db.pgbouncer('stop')
                # We need to stop archiving WAL because when network connectivity
                # returns, it can be another master in cluster. We need to stop
                # archiving to prevent "wrong" WAL appears in archive.
                self.db.stop_archiving_wal()
            else:
                self.start_pooler()
            logging.warning('Lock in ZK is released but could not be ' 'acquired. Reconnecting to ZK.')
            self.zk.reconnect()
        elif holder != my_hostname:
            self.db.pgbouncer('stop')
            logging.warning('Lock in ZK is being held by %s. We ' 'should return to cluster here.', holder)
            self._return_to_cluster(holder, 'master')

    def write_host_stat(self, hostname, db_state):
        stream_from = self.config.get('global', 'stream_from')
        replics_info = db_state.get('replics_info')
        self._fix_hostname(replics_info)
        wal_receiver_info = db_state['wal_receiver']
        host_path = '{member_path}/{hostname}'.format(member_path=zk.MEMBERS_PATH, hostname=hostname)
        replics_info_path = '{host_path}/replics_info'.format(host_path=host_path)
        ha_path = '{host_path}/ha'.format(host_path=host_path)
        wal_receiver_path = '{host_path}/wal_receiver'.format(host_path=host_path)
        if not stream_from:
            if not self.zk.ensure_path(ha_path):
                logging.warning('Could not write ha host in ZK.')
                return False
        else:
            if self.zk.exists_path(ha_path) and not self.zk.delete(ha_path):
                logging.warning('Could not delete ha host in ZK.')
                return False
        if wal_receiver_info is not None:
            if not self.zk.write(wal_receiver_path, wal_receiver_info, preproc=json.dumps, need_lock=False):
                logging.warning('Could not write host wal_receiver_info to ZK.')
                return False
        if replics_info is not None:
            if not self.zk.write(replics_info_path, replics_info, preproc=json.dumps, need_lock=False):
                logging.warning('Could not write host replics_info to ZK.')
                return False

    def remove_stale_operation(self, hostname):
        op_path = '%s/%s/op' % (zk.MEMBERS_PATH, hostname)
        last_op = self.zk.noexcept_get(op_path)
        if self.is_op_destructive(last_op):
            logging.warning('Stale operation %s detected. ' 'Removing track from zk.', last_op)
            self.zk.delete(op_path)

    def start_pooler(self):
        start_pgbouncer = self.config.getboolean('replica', 'start_pgbouncer')
        if not self.db.pgbouncer('status') and start_pgbouncer:
            self.db.pgbouncer('start')

    def get_replics_info(self, zk_state):
        stream_from = self.config.get('global', 'stream_from')
        if stream_from:
            replics_info_path = '{member_path}/{hostname}/replics_info'.format(
                member_path=zk.MEMBERS_PATH, hostname=stream_from
            )
            replics_info = self.zk.noexcept_get(replics_info_path, preproc=json.loads)
        else:
            replics_info = zk_state['replics_info']
        return replics_info

    def change_master(self, db_state, master):
        logging.warning(
            'Seems that master has been switched to %s '
            'while we are streaming WAL from %s. '
            'We should re-master '
            'here.',
            master,
            db_state['master_fqdn'],
        )
        return self._return_to_cluster(master, 'replica')

    def replica_return(self, db_state, zk_state):
        my_hostname = helpers.get_hostname()
        self.write_host_stat(my_hostname, db_state)
        holder = zk_state['lock_holder']

        self.checks['failover'] = 0
        limit = self.config.getfloat('replica', 'recovery_timeout')

        # Try to resume WAL replaying, it can be paused earlier
        self.db.pg_wal_replay_resume()

        if not self._check_archive_recovery(limit) and not self._wait_for_streaming(limit):
            # Wal receiver is not running and
            # postgresql isn't in archive recovery
            # We should try to restart
            logging.warning('We should try re-master one more ' 'time here.')
            return self._return_to_cluster(holder, 'replica', is_dead=False)

    def _get_streaming_replica_from_replics_info(self, fqdn, replics_info):
        if not replics_info:
            return None
        for replica in replics_info:
            if replica['client_hostname'] == fqdn and replica['state'] == 'streaming':
                return replica
        return None

    def non_ha_replica_iter(self, db_state, zk_state):
        try:
            logging.info('Current replica is non ha.')
            if not zk_state['alive']:
                return None
            my_hostname = helpers.get_hostname()
            self.remove_stale_operation(my_hostname)
            self.write_host_stat(my_hostname, db_state)
            stream_from = self.config.get('global', 'stream_from')
            can_delayed = self.config.getboolean('replica', 'can_delayed')
            replics_info = self.get_replics_info(zk_state) or []
            self.checks['failover'] = 0
            streaming = self._get_streaming_replica_from_replics_info(my_hostname, replics_info) and bool(
                db_state['wal_receiver']
            )
            streaming_from_master = self._get_streaming_replica_from_replics_info(
                my_hostname, zk_state.get('replics_info')
            ) and bool(db_state['wal_receiver'])
            logging.error(
                'Streaming: %s, streaming from master: %s, wal_receiver: %s, replics_info: %s',
                streaming,
                streaming_from_master,
                db_state['wal_receiver'],
                replics_info,
            )
            if not streaming and not can_delayed:
                logging.warning('Seems that we are not really streaming WAL from %s.', stream_from)
                self._replication_manager.leave_sync_group()
                replication_source_is_dead = self._check_master_is_really_dead(stream_from)
                replication_source_replica_info = self._get_streaming_replica_from_replics_info(
                    stream_from, zk_state.get('replics_info')
                )
                wal_receiver_info = self._zk_get_wal_receiver_info(stream_from)
                replication_source_streams = bool(
                    wal_receiver_info and wal_receiver_info[0].get('status') == 'streaming'
                )
                logging.error(replication_source_replica_info)
                current_master = zk_state['lock_holder']
                if replication_source_is_dead:
                    # Replication source is dead. We need to streaming from master while it became alive and start streaming from master.
                    if stream_from == current_master or current_master is None:
                        logging.warning(
                            'My replication source %s seems dead and it was master. Waiting new master appears in cluster or old became alive.',
                            stream_from,
                        )
                    elif not streaming_from_master:
                        logging.warning(
                            'My replication source %s seems dead. Try to stream from master %s',
                            stream_from,
                            current_master,
                        )
                        return self._return_to_cluster(current_master, 'replica', is_dead=False)
                    else:
                        logging.warning(
                            'My replication source %s seems dead. We are already streaming from master %s. Waiting replication source became alive.',
                            stream_from,
                            current_master,
                        )
                else:
                    # Replication source is alive. We need to wait while it starts streaming from master and start streaming from it.
                    if replication_source_streams:
                        logging.warning(
                            'My replication source %s seems alive and streams, try to stream from it',
                            stream_from,
                        )
                        return self._return_to_cluster(stream_from, 'replica', is_dead=False)
                    elif stream_from == current_master:
                        logging.warning(
                            'My replication source %s seems alive and it is current master, try to stream from it',
                            stream_from,
                        )
                        return self._return_to_cluster(stream_from, 'replica', is_dead=False)
                    else:
                        logging.warning(
                            'My replication source %s seems alive. But it don\'t streaming. Waiting it starts streaming from master.',
                            stream_from,
                        )
            self.checks['remaster'] = 0
            self.start_pooler()
            self._reset_simple_remaster_try()
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return None

    def _accept_switchover(self, lock_holder, previous_master):
        if not self._can_do_switchover():
            return None

        # WARNING: we shouldn't allow multiple hosts to enter this branch
        if not self.zk.write(zk.SWITCHOVER_STATE_PATH, 'candidate_found', need_lock=False):
            logging.error('Failed to state that we are the new master candidate in ZK.')
            return None

        #
        # All checks are done. Waiting for master shutdown, acquiring the lock in ZK,
        # promoting and writing last switchover timestamp to ZK.
        #
        limit = self.config.getfloat('global', 'postgres_timeout')
        # Current master is lock holder. Otherwise consider last master as current.
        current_master = lock_holder or previous_master
        if current_master is not None and not helpers.await_for(
            lambda: self._check_master_is_really_dead(current_master), limit, 'master is down'
        ):
            return None

        # Wait switchover_master_shut state only if current master is alive, i.e. lock holder exists.
        if lock_holder is not None and not helpers.await_for(
            lambda: self.zk.get(zk.FAILOVER_INFO_PATH) == 'switchover_master_shut',
            limit,
            'failover state is switchover_master_shut',
        ):
            # Mark switchover node as failure
            self.zk.write(zk.SWITCHOVER_STATE_PATH, 'master_timed_out', need_lock=False)
            return False

        if not self.zk.try_acquire_lock(allow_queue=True, timeout=limit):
            logging.info('Could not acquire lock in ZK. ' 'Not doing anything.')
            return None

        if not self._do_failover():
            return False

        self._cleanup_switchover()
        self.zk.write(zk.LAST_SWITCHOVER_TIME_PATH, time.time())

    def replica_iter(self, db_state, zk_state):
        """
        Iteration if local postgresql is replica
        """
        try:
            if not zk_state['alive']:
                return None
            my_hostname = helpers.get_hostname()
            self.remove_stale_operation(my_hostname)
            holder = zk_state['lock_holder']
            self.write_host_stat(my_hostname, db_state)

            if self._is_single_node:
                logging.error("HA replica shouldn't exist inside a single node cluster")
                return None

            replics_info = zk_state['replics_info']
            streaming = False
            for i in replics_info or []:
                if i['client_hostname'] != my_hostname:
                    continue
                if i['state'] == 'streaming':
                    streaming = True

            if self._detect_replica_switchover():
                logging.warning('Planned switchover condition detected')
                self._replication_manager.enter_sync_group(replica_infos=replics_info)
                return self._accept_switchover(holder, db_state.get('master_fqdn'))

            # If there is no master lock holder and it is not a switchover
            # then we should consider current cluster state as failover.
            if holder is None:
                logging.error('According to ZK master has died. We should ' 'verify it and do failover if possible.')
                return self._accept_failover()

            self.checks['failover'] = 0

            if holder != db_state['master_fqdn'] and holder != my_hostname:
                self._replication_manager.leave_sync_group()
                return self.change_master(db_state, holder)

            self.db.ensure_replaying_wal()

            if not streaming:
                logging.warning('Seems that we are not really streaming WAL from %s.', holder)
                self._replication_manager.leave_sync_group()

                return self.replica_return(db_state, zk_state)

            self.checks['remaster'] = 0

            self.start_pooler()
            self._reset_simple_remaster_try()

            self._replication_manager.enter_sync_group(replica_infos=replics_info)
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return None

    def dead_iter(self, db_state, zk_state):
        """
        Iteration if local postgresql is dead
        """
        if not zk_state['alive'] or db_state['alive']:
            return None

        self.db.pgbouncer('stop')

        self._replication_manager.leave_sync_group()
        self.zk.release_if_hold(zk.MASTER_LOCK_PATH)

        role = self.db.role
        last_tli = self.db.get_data_from_control_file('Latest checkpoint.s TimeLineID', preproc=int, log=False)
        last_master = None
        if role == 'replica' and db_state.get('prev_state'):
            last_master = db_state['prev_state'].get('master_fqdn')

        holder = self.zk.get_current_lock_holder()
        if holder and holder != helpers.get_hostname():
            if role == 'replica' and holder == last_master:
                logging.info('Seems that master has not changed but ' 'PostgreSQL is dead. Starting it.')
                return self.db.start_postgresql()

            #
            # We can get here in two cases:
            # We were master and now we are dead.
            # We were replica, master has changed and now we are dead.
            #
            logging.warning(
                'Seems that master is %s and local PostgreSQL is ' 'dead. We should return to cluster here.', holder
            )
            return self._return_to_cluster(holder, role, is_dead=True)

        else:
            #
            # The only case we get here is absence of master (no one holds the
            # lock) and our PostgreSQL is dead.
            #
            logging.error('Seems that all hosts (including me) are dead. ' 'Trying to start PostgreSQL.')
            if role == 'master':
                zk_timeline = zk_state['timeline']
                if zk_timeline is not None and zk_timeline != last_tli:
                    logging.error(
                        'Seems that I was master before but not ' 'the last one in the cluster. Not doing ' 'anything.'
                    )
                    return None
            #
            # Role was master. We need to disable archive_command before
            # starting postgres to prevent "wrong" last WAL in archive.
            #
            self.db.stop_archiving_wal_stopped()
            return self.db.start_postgresql()

    def _drop_stale_switchover(self, db_state):
        if not self.zk.try_acquire_lock(zk.SWITCHOVER_LOCK_PATH):
            return
        try:
            switchover_info = self.zk.get(zk.SWITCHOVER_MASTER_PATH, preproc=json.loads)
            if not switchover_info:
                return
            switchover_state = self.zk.get(zk.SWITCHOVER_STATE_PATH)
            if (
                switchover_state != 'scheduled'
                or switchover_info.get('timeline') is None
                or switchover_info['timeline'] < db_state['timeline']
            ):
                logging.warning('Dropping stale switchover')
                logging.debug(
                    'Switchover info: state %s; info %s; db timeline %s',
                    switchover_state,
                    switchover_info,
                    db_state['timeline'],
                )
                self._cleanup_switchover()
        finally:
            # We want to release this lock regardless of what happened in 'try' block
            self.zk.release_lock(zk.SWITCHOVER_LOCK_PATH)

    def _cleanup_switchover(self):
        self.zk.delete(zk.SWITCHOVER_LSN_PATH)
        self.zk.delete(zk.SWITCHOVER_MASTER_PATH)
        self.zk.delete(zk.SWITCHOVER_STATE_PATH)
        self.zk.delete(zk.FAILOVER_INFO_PATH)

    def _update_single_node_status(self, role):
        """
        In case if current role is 'master', we should determine new status
        and update it locally and in ZK.
        Otherwise, we should just update the status from ZK
        """
        if role == 'master':
            ha_hosts = zk.get_ha_hosts(self.zk)
            if ha_hosts is None:
                logging.error('Failed to update single node status because of empty ha host list.')
                return
            self._is_single_node = len(ha_hosts) == 1
            if self._is_single_node:
                self.zk.ensure_path(zk.SINGLE_NODE_PATH)
            else:
                self.zk.delete(zk.SINGLE_NODE_PATH)
        else:
            self._is_single_node = self.zk.exists_path(zk.SINGLE_NODE_PATH)

    def _verify_timeline(self, db_state, zk_state):
        """
        Make sure current timeline corresponds to the rest of the cluster (@ZK).
        Save timeline and some related info into zk
        """
        # Skip if role is not master
        if self.db.role != 'master':
            logging.error('We are not master. Not doing anything.')
            return None

        # Establish whether local timeline corresponds to master timeline at ZK.
        tli_res = zk_state['timeline'] == db_state['timeline']
        # If it does, but there is no info on replicas,
        # close local PG instance.
        if tli_res:
            if zk_state['replics_info_written'] is False:
                logging.error('Some error with ZK.')
                # Actually we should never get here but checking it just in case.
                # Here we should end iteration and check and probably close master
                # at the begin of master_iter
                return None
        # If ZK does not have timeline info, write it.
        elif zk_state['timeline'] is None:
            logging.warning('Could not get timeline from ZK. Saving it.')
            self.zk.write(zk.TIMELINE_INFO_PATH, db_state['timeline'])
        # If there is a mismatch in timeline:
        # - If ZK timeline is greater than local, there must be another master.
        #   In that case local instance have no business holding the lock.
        # - If local timeline is greater, local instance has likely been
        #   promoted recently.
        #   Update ZK structure to reflect that.
        elif tli_res is False:
            self.db.checkpoint()
            zk_tli = zk_state['timeline']
            db_tli = db_state['timeline']
            if zk_tli and zk_tli > db_tli:
                logging.error('ZK timeline is newer than local.' 'Releasing leader lock')
                self.db.pgbouncer('stop')

                self.zk.release_lock()
                #
                # This timeout is needed for master with newer timeline
                # to acquire the lock in ZK.
                #
                time.sleep(10 * self.config.getfloat('global', 'iteration_timeout'))
                return None
            elif zk_tli and zk_tli < db_tli:
                logging.warning('Timeline in ZK is older than ours. Updating ' 'it it ZK.')
                self.zk.write(zk.TIMELINE_INFO_PATH, db_tli)
        logging.debug('Timeline verification succeded')
        return True

    def _reset_simple_remaster_try(self):
        simple_remaster_path = zk.get_simple_remaster_try_path(get_hostname())
        if self.zk.noexcept_get(simple_remaster_path) != 'no':
            self.zk.noexcept_write(simple_remaster_path, 'no', need_lock=False)

    def _set_simple_remaster_try(self):
        simple_remaster_path = zk.get_simple_remaster_try_path(get_hostname())
        self.zk.noexcept_write(simple_remaster_path, 'yes', need_lock=False)

    def _is_simple_remaster_tried(self):
        if self.zk.noexcept_get(zk.get_simple_remaster_try_path(get_hostname())) == 'yes':
            return True
        return False

    def _try_simple_remaster_with_lock(self, *args, **kwargs):
        if not self.config.getboolean('global', 'do_consecutive_remaster'):
            return self._simple_remaster(*args, **kwargs)
        lock_holder = self.zk.get_current_lock_holder(zk.REMASTER_LOCK_PATH)
        if (
            lock_holder is None and not self.zk.try_acquire_lock(zk.REMASTER_LOCK_PATH)
        ) or lock_holder != helpers.get_hostname():
            return True
        result = self._simple_remaster(*args, **kwargs)
        self.zk.release_lock(zk.REMASTER_LOCK_PATH)
        return result

    def _simple_remaster(self, limit, new_master, is_dead):
        remaster_checks = self.config.getint('replica', 'remaster_checks')
        need_restart = self.config.getboolean('replica', 'remaster_restart')

        logging.info('Starting simple remaster.')
        if self.checks['remaster'] >= remaster_checks:
            self._set_simple_remaster_try()

        if need_restart and not is_dead and self.db.stop_postgresql(timeout=limit) != 0:
            logging.error('Could not stop PostgreSQL. Will retry.')
            self.checks['remaster'] = 0
            return True

        if self.db.recovery_conf('create', new_master) != 0:
            logging.error('Could not generate recovery.conf. Will retry.')
            self.checks['remaster'] = 0
            return True

        if not is_dead and not need_restart:
            if not self.db.reload():
                logging.error('Could not reload PostgreSQL. Skipping it.')
            self.db.ensure_replaying_wal()
        else:
            if self.db.start_postgresql() != 0:
                logging.error('Could not start PostgreSQL. Skipping it.')

        if self._wait_for_recovery(limit) and self._check_archive_recovery(limit):
            #
            # We have reached consistent state but there is a small
            # chance that we are not streaming changes from new master
            # with: "new timeline N forked off current database system
            # timeline N-1 before current recovery point M".
            # Checking it with the info from ZK.
            #
            if self._wait_for_streaming(limit, new_master):
                #
                # The easy way succedeed.
                #
                logging.info('Simple remastering succeded.')
                self._remaster_handle_slots()
                return True
            else:
                return False

    def _rewind_remaster(self, is_postgresql_dead, limit, new_master):
        logging.info("Starting pg_rewind")

        if not helpers.await_for(
            lambda: not self._check_master_is_really_dead(new_master), limit, 'master alive and ready for rewind'
        ):
            return None

        if not self.zk.write('%s/%s/op' % (zk.MEMBERS_PATH, helpers.get_hostname()), 'rewind', need_lock=False):
            logging.error('Unable to save destructive op state: rewind')
            return None

        self.db.pgbouncer('stop')

        if not is_postgresql_dead and self.db.stop_postgresql(timeout=limit) != 0:
            logging.error('Could not stop PostgreSQL. Will retry.')
            return None

        self.checks['rewind'] += 1
        if self.db.do_rewind(new_master) != 0:
            logging.error('Error while using pg_rewind. Will retry.')
            return True

        # Rewind has finished successfully so we can drop its operation node
        self.zk.delete('%s/%s/op' % (zk.MEMBERS_PATH, helpers.get_hostname()))
        return self._attach_to_master(new_master, limit)

    def _attach_to_master(self, new_master, limit):
        """
        Generate recovery.conf and start PostgreSQL.
        """
        logging.info('Converting role to replica of %s.', new_master)
        if self.db.recovery_conf('create', new_master) != 0:
            logging.error('Could not generate recovery.conf. Will retry.')
            self.checks['remaster'] = 0
            return None

        if self.db.start_postgresql() != 0:
            logging.error('Could not start PostgreSQL. Skipping it.')

        if not self._wait_for_recovery(limit):
            self.checks['remaster'] = 0
            return None

        self._remaster_handle_slots()

        if not self._wait_for_streaming(limit):
            self.checks['remaster'] = 0
            return None

        logging.info('Seems, that returning to cluster succeeded. ' 'Unbelievable!')
        self.db.checkpoint()
        return True

    def _remaster_handle_slots(self):
        need_slots = self.config.getboolean('global', 'use_replication_slots')
        if need_slots:
            my_hostname = helpers.get_hostname()
            hosts = self.zk.get_children(zk.MEMBERS_PATH)
            if hosts:
                if my_hostname in hosts:
                    hosts.remove(my_hostname)
                hosts = [i.replace('.', '_').replace('-', '_') for i in hosts]
                logging.debug(hosts)
                if not self.db.replication_slots('drop', hosts):
                    logging.warning('Could not drop replication slots. Do not ' 'forget to do it manually!')
            else:
                logging.warning(
                    'Could not get all hosts list from ZK. '
                    'Replication slots should be dropped but we '
                    'are unable to do it. Skipping it.'
                )

    def _get_db_state(self):
        state = self.db.get_data_from_control_file('Database cluster state')
        if not state or state == '':
            logging.error('Could not get info from controlfile about current ' 'cluster state.')
            return None
        logging.info('Database cluster state is: %s' % state)
        return state

    def _return_to_cluster(self, new_master, role, is_dead=False):
        """
        Return to cluster (try stupid method, if it fails we try rewind)
        """
        logging.info('Starting returning to cluster.')
        if self.checks['remaster'] >= 0:
            self.checks['remaster'] += 1
        else:
            self.checks['remaster'] = 1
        logging.debug("Remaster_checks is %d", self.checks['remaster'])

        failover_state = self.zk.noexcept_get(zk.FAILOVER_INFO_PATH)
        if failover_state is not None and failover_state not in ('finished', 'promoting', 'checkpointing'):
            logging.info(
                'We are not able to return to cluster since ' 'failover is still in progress - %s.', failover_state
            )
            return None

        limit = self.config.getfloat('replica', 'recovery_timeout')
        try:
            #
            # First we try to know if the cluster
            # has been turned off correctly.
            #
            state = self._get_db_state()
            if not state:
                return None

            #
            # If we are alive replica, we should first try an easy way:
            # stop PostgreSQL, regenerate recovery.conf, start PostgreSQL
            # and wait for recovery to finish. If last fails within
            # a reasonable time, we should go a way harder (see below).
            # Simple remaster will not work if we were promoting or
            # rewinding and failed. So only hard way possible in this case.
            #
            last_op = self.zk.noexcept_get('%s/%s/op' % (zk.MEMBERS_PATH, helpers.get_hostname()))
            logging.info('Last op is: %s' % str(last_op))
            if role != 'master' and not self.is_op_destructive(last_op) and not self._is_simple_remaster_tried():
                logging.info('Trying to do a simple remaster.')
                result = self._try_simple_remaster_with_lock(limit, new_master, is_dead)
                logging.info('Remaster count: %s finish with result: %s', self.checks['remaster'], result)
                return None

            #
            # If our rewind attempts fail several times
            # we should create special flag-file, stop posgresql and then exit.
            #
            max_rewind_retries = self.config.getint('global', 'max_rewind_retries')
            if self.checks['rewind'] > max_rewind_retries:
                self.db.pgbouncer('stop')
                self.db.stop_postgresql(timeout=limit)
                work_dir = self.config.get('global', 'working_dir')
                fname = '%s/.pgsync_rewind_fail.flag' % work_dir
                with open(fname, 'w') as fobj:
                    fobj.write(str(time.time()))
                logging.error('Could not rewind %d times. Exiting.', max_rewind_retries)
                sys.exit(1)

            #
            # The hard way starts here.
            #
            if not self._rewind_remaster(is_dead, limit, new_master):
                return None

        except Exception:
            logging.error('Unexpected error while trying to ' 'return to cluster. Exiting.')
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            sys.exit(1)

    def _promote(self):
        if not self.zk.write(zk.FAILOVER_INFO_PATH, 'promoting'):
            logging.error('Could not write failover state to ZK.')
            return False

        if not self.zk.write(zk.CURRENT_PROMOTING_HOST, helpers.get_hostname()):
            logging.error('Could not write self as last promoted host.')
            return False

        if not self.db.promote():
            logging.error('Could not promote me as a new master. We should ' 'release the lock in ZK here.')
            # We need to close here and recheck postgres role. If it was no actual
            # promote, we need too delete self as last promoted host, mark failover "finished"
            # and return to cluster. If self master we need to continue promote despite on exit code
            # because self already accepted some data modification which will be loss if
            # we simply return False here.
            if self.db.get_role() != 'master':
                self.db.pgbouncer('stop')
                if not self.zk.delete(zk.CURRENT_PROMOTING_HOST):
                    logging.error('Could not remove self as current promoting host.')
                if not self.zk.write(zk.FAILOVER_INFO_PATH, 'finished'):
                    logging.error('Could not write failover state to ZK.')
                return False

            logging.info('Promote command failed but we are current master. Continue')

        if not self.zk.noexcept_write(zk.FAILOVER_INFO_PATH, 'checkpointing'):
            logging.warning('Could not write failover state to ZK.')

        logging.debug('Doing checkpoint after promoting.')
        if not self.db.checkpoint(query=self.config.get('debug', 'promote_checkpoint_sql', fallback=None)):
            logging.warning('Could not checkpoint after failover.')

        my_tli = self.db.get_data_from_control_file('Latest checkpoint.s TimeLineID', preproc=int, log=False)

        if not self.zk.write(zk.TIMELINE_INFO_PATH, my_tli):
            logging.warning('Could not write timeline to ZK.')

        if not self.zk.write(zk.FAILOVER_INFO_PATH, 'finished'):
            logging.error('Could not write failover state to ZK.')

        if not self.zk.delete(zk.CURRENT_PROMOTING_HOST):
            logging.error('Could not remove self as current promoting host.')

        return True

    def _promote_handle_slots(self):
        need_slots = self.config.getboolean('global', 'use_replication_slots')
        if need_slots:
            if not self.zk.write(zk.FAILOVER_INFO_PATH, 'creating_slots'):
                logging.warning('Could not write failover state to ZK.')

            hosts = self._get_ha_replics()
            if hosts is None:
                logging.error(
                    'Could not get all hosts list from ZK. '
                    'Replication slots should be created but we '
                    'are unable to do it. Releasing the lock.'
                )
                return False

            hosts = [i.replace('.', '_').replace('-', '_') for i in hosts]
            if not self.db.replication_slots('create', hosts):
                logging.error('Could not create replication slots. Releasing ' 'the lock in ZK.')
                return False

        return True

    def _check_my_timeline_sync(self):
        my_tli = self.db.get_data_from_control_file('Latest checkpoint.s TimeLineID', preproc=int, log=False)
        try:
            zk_tli = self.zk.get(zk.TIMELINE_INFO_PATH, preproc=int)
        except ZookeeperException:
            logging.error('Could not get timeline from ZK.')
            return False
        if zk_tli is None:
            logging.warning('There was no timeline in ZK. Skipping this check.')
        elif zk_tli != my_tli:
            logging.error(
                'My timeline (%d) differs from timeline in ZK (%d). ' 'Checkpointing and skipping iteration.',
                my_tli,
                zk_tli,
            )
            self.db.checkpoint()
            return False
        return True

    def _check_last_failover_timeout(self):
        try:
            last_failover_ts = self.zk.get(zk.LAST_FAILOVER_TIME_PATH, preproc=float)
        except ZookeeperException:
            logging.error('Can\'t get last failover time from ZK.')
            return False

        if last_failover_ts is None:
            logging.warning('There was no last failover ts in ZK. Skipping this check.')
            last_failover_ts = 0.0
        diff = time.time() - last_failover_ts
        if not helpers.check_last_failover_time(last_failover_ts, self.config):
            logging.info('Last time failover has been done %f seconds ago. ' 'Not doing anything.', diff)
            return False
        logging.info('Last failover has been done %f seconds ago.', diff)
        return True

    def _check_master_unavailability_timeout(self):
        previous_master_availability_time = self.zk.noexcept_get(zk.LAST_MASTER_AVAILABILITY_TIME, preproc=float)
        if previous_master_availability_time is None:
            logging.error('Failed to get last master availability time.')
            return False
        time_passed = time.time() - previous_master_availability_time
        if time_passed < self.config.getfloat('replica', 'master_unavailability_timeout'):
            logging.info('Last time we seen master %f seconds ago, not doing anything.', time_passed)
            return False
        return True

    def _is_older_then_master(self):
        try:
            lsn = self.zk.get(zk.SWITCHOVER_LSN_PATH)
            # If there is no lsn in ZK it means that master is dead
            if lsn is None:
                return True
            # Our LSN should be greater than LSN in master's pg_control
            # because of shutdown record. For more info about address:
            # https://www.postgresql.org/message-id/flat/A7683985-2EC2-40AD-AAAC-B44BD0F29723%40simply.name
            return self.db.get_replay_diff(lsn) > 0
        except ZookeeperException:
            return False

    def _can_do_failover(self):
        autofailover = self.config.getboolean('global', 'autofailover')

        if not autofailover:
            logging.info("Autofailover is disabled. Not doing anything.")
            return False

        if not self._check_my_timeline_sync():
            return False

        if not self._check_last_failover_timeout():
            return False
        if not self._check_master_is_really_dead():
            logging.warning(
                'According to ZK master has died but it is ' 'still accessible through libpq. Not doing ' 'anything.'
            )
            return False
        if not self._check_master_unavailability_timeout():
            return False
        if self.db.is_replaying_wal(self.config.getfloat('global', 'iteration_timeout')):
            logging.info("Host is still replaying WAL, so it can't be promoted.")
            return False

        replica_infos = self.zk.noexcept_get(zk.REPLICS_INFO_PATH, preproc=json.loads)
        if replica_infos is None:
            logging.error('Unable to get replics info from ZK.')
            return False

        allow_data_loss = self.config.getboolean('replica', 'allow_potential_data_loss')
        logging.info(f'Data loss is: {allow_data_loss}')
        is_promote_safe = self._replication_manager.is_promote_safe(
            zk.get_alive_hosts(self.zk),
            replica_infos=replica_infos,
        )
        if not allow_data_loss and not is_promote_safe:
            logging.warning('Promote is not allowed with given configuration.')
            return False
        self.db.pg_wal_replay_pause()
        election_timeout = self.config.getint('global', 'election_timeout')
        priority = self.config.getint('global', 'priority')
        election = FailoverElection(
            self.zk,
            election_timeout,
            replica_infos,
            self._replication_manager,
            allow_data_loss,
            priority,
            self.db.get_wal_receive_lsn(),
            len(helpers.make_current_replics_quorum(replica_infos, zk.get_alive_hosts(self.zk, election_timeout / 2))),
        )
        try:
            return election.make_election()
        except (ZookeeperException, ElectionError):
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return False

    def _get_switchover_candidate(self):
        switchover_info = self.zk.get(zk.SWITCHOVER_MASTER_PATH, preproc=json.loads)
        if switchover_info is None:
            return None

        if switchover_info.get('destination') is not None:
            return switchover_info.get('destination')
        else:
            replica_infos = self._get_extended_replica_infos()
            if replica_infos is None:
                return None
            if self.config.getboolean('replica', 'allow_potential_data_loss'):
                return helpers.get_oldest_replica(replica_infos)
            else:
                return self._replication_manager.get_ensured_sync_replica(replica_infos)

    def _get_extended_replica_infos(self):
        replica_infos = self.zk.get(zk.REPLICS_INFO_PATH, preproc=json.loads)
        if replica_infos is None:
            logging.error('Unable to get replica infos from ZK.')
            return None
        for info in replica_infos:
            hostname = info['client_hostname']
            info['priority'] = self.zk.get(zk.get_host_prio_path(hostname), preproc=int)
        return replica_infos

    def _can_do_switchover(self):
        if not self._is_older_then_master():
            return False

        if not self._check_my_timeline_sync():
            return False

        switchover_candidate = self._get_switchover_candidate()

        # Make sanity check of switchover conditions, and proceed to
        # promotion immediately without failover or dead master checks.
        if switchover_candidate != helpers.get_hostname():
            logging.info(
                f"Switchover candidate is: {switchover_candidate}. " "We are not a candidate, so we can't promote."
            )
            return False

        logging.info('We are switchover candidate, so we have to promote here.')
        # If master is alive and it participates in switchover, then we can proceed
        if self.zk.get(zk.FAILOVER_INFO_PATH) == 'switchover_initiated':
            return True
        # If master is dead but we can't failover, then we also proceed
        if self.zk.get_current_lock_holder(zk.MASTER_LOCK_PATH) is None:
            return True
        logging.warning("Master holds the lock but didn't initiate switchover yet. " "Waiting for it...")
        return False

    def _accept_failover(self):
        """
        Failover magic is here
        """
        try:
            if not self._can_do_failover():
                return None

            #
            # All checks are done. Acquiring the lock in ZK, promoting and
            # writing last failover timestamp to ZK.
            #
            if not self.zk.try_acquire_lock():
                logging.info('Could not acquire lock in ZK. ' 'Not doing anything.')
                return None
            self.db.pg_wal_replay_resume()

            if not self._do_failover():
                return False

            self.zk.write(zk.LAST_FAILOVER_TIME_PATH, time.time())
        except Exception:
            logging.error('Unexpected error while trying to do failover. ' 'Exiting.')
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            sys.exit(1)

    def _do_failover(self):
        if not self.zk.delete(zk.FAILOVER_INFO_PATH):
            logging.error('Could not remove previous failover state. ' 'Releasing the lock.')
            self.zk.release_lock()
            return False

        if not self._promote_handle_slots() or not self._promote():
            self.zk.release_lock()
            return False
        self._replication_manager.leave_sync_group()
        return True

    def _wait_for_recovery(self, limit=-1):
        """
        Stop until postgresql complete recovery.
        With limit=-1 the loop here can be infinite.
        """

        def check_recovery_completion():
            if self.db.is_alive():
                logging.debug('PostgreSQL has completed recovery.')
                return True
            if self.db.get_postgresql_status() != 0:
                logging.error('PostgreSQL service seems to be dead. ' 'No recovery is possible in this case.')
                return False
            return None

        return helpers.await_for_value(check_recovery_completion, limit, "PostgreSQL has completed recovery")

    def _check_archive_recovery(self, limit):
        """
        Returns True if postgresql is in recovery from archive
        and False if it hasn't started recovery within `limit` seconds
        """

        def check_recovery_start():
            if self._check_postgresql_streaming():
                logging.debug('PostgreSQL is already streaming from master')
                return True

            # we can get here with another role or
            # have role changed during this retrying cycle
            role = self.db.get_role()
            if role != 'replica':
                logging.warning('PostgreSQL role changed during archive recovery check. Now it doesn\'t make sense')
                self.db.pgbouncer('stop')
                return False

            if self.db.is_replaying_wal(1):
                logging.debug('PostgreSQL is in archive recovery')
                return True
            return None

        return helpers.await_for_value(check_recovery_start, limit, 'PostgreSQL started archive recovery')

    def _get_replics_info_from_zk(self, master):
        if master:
            replics_info_path = '{member_path}/{hostname}/replics_info'.format(
                member_path=zk.MEMBERS_PATH, hostname=master
            )
        else:
            replics_info_path = zk.REPLICS_INFO_PATH
        return self.zk.get(replics_info_path, preproc=json.loads)

    @staticmethod
    def _is_caught_up(replica_infos):
        for replica in replica_infos:
            if replica['client_hostname'] == helpers.get_hostname() and replica['state'] == 'streaming':
                return True
        return False

    def _check_postgresql_streaming(self, master=None):
        if not self.db.is_alive():
            logging.error('PostgreSQL is dead. Waiting for streaming ' 'is useless.')
            return False

        # we can get here with another role or
        # have role changed during this retrying cycle
        if self.db.get_role() != 'replica':
            self.db.pgbouncer('stop')
            logging.warning("PostgreSQL is not a replica, so it can't be streaming.")
            return False

        try:
            replica_infos = self._get_replics_info_from_zk(master)
        except ZookeeperException:
            logging.error("Can't get replics_info from ZK. Won't wait for timeout.")
            return False

        if replica_infos is not None and (
            Pgsync._is_caught_up(replica_infos) and self.db.check_walreceiver(replica_infos)
        ):
            logging.debug('PostgreSQL has started streaming from master.')
            return True

        return None

    def _wait_for_streaming(self, limit=-1, master=None):
        """
        Stop until postgresql start streaming from master.
        With limit=-1 the loop here can be infinite.
        """
        check_streaming = functools.partial(self._check_postgresql_streaming, master)
        return helpers.await_for_value(check_streaming, limit, 'PostgreSQL started streaming from master')

    def _wait_for_lock(self, lock, limit=-1):
        """
        Wait until lock acquired
        """

        def is_lock_acquired():
            if self.zk.try_acquire_lock(lock):
                return True
            # There is a chance that our connection with ZK is dead
            # (and that is actual reason of not getting lock).
            # So we reinit connection here.
            self.re_init_zk()
            return False

        return helpers.await_for(is_lock_acquired, limit, f'acquired {lock} lock in ZK')

    def _check_master_is_really_dead(self, master=None):
        """
        Returns True if master is not accessible via postgres protocol
        and False otherwise
        """
        if not master:
            master = self.db.recovery_conf('get_master')
            if not master:
                return False
        append = self.config.get('global', 'append_master_conn_string')
        try:
            conn = psycopg2.connect('host=%s %s' % (master, append))
            conn.autocommit = True
            cur = conn.cursor()
            cur.execute('SELECT 42')
            if cur.fetchone()[0] == 42:
                return False
            return True
        except Exception as err:
            logging.debug('%s while trying to check master health.', str(err))
            return True

    def _get_ha_replics(self):
        hosts = zk.get_ha_hosts(self.zk)
        if not hosts:
            return None
        my_hostname = helpers.get_hostname()
        if my_hostname in hosts:
            hosts.remove(my_hostname)
        return set(hosts)

    def _get_zk_members(self):
        """
        Checks the presence of subnodes in MEMBERS_PATH at ZK.
        """
        while True:
            timer = IterationTimer()
            self.zk.ensure_path(zk.MEMBERS_PATH)
            members = self.zk.get_children(zk.MEMBERS_PATH)
            if members is not None:
                return members
            self.re_init_zk()
            timer.sleep(self.config.getfloat('global', 'iteration_timeout'))

    def _check_master_switchover(self, db_state, zk_state):
        """
        Check if scheduled switchover is initiated.
        Perform sanity check on current local and cluster condition.
        Abort or postpone switchover if any of them fail.
        """
        switchover_info = zk_state['switchover']

        # Scheduled switchover node exists.
        if not switchover_info:
            return None

        # The node contains hostname of current instance
        if switchover_info.get('hostname') != helpers.get_hostname():
            return None

        # Current instance is master
        if self.db.get_role() != 'master':
            logging.error('Current role is %s, but switchover requested.', self.db.get_role())
            return None

        # There were no failed attempts in the past
        state = self.zk.get(zk.SWITCHOVER_STATE_PATH)
        # Ignore silently if node does not exist
        if state is None:
            return None
        # Ignore failed or in-progress switchovers
        if state != 'scheduled':
            logging.warning('Switchover state is %s, will not proceed.', state)
            return None

        # Timeline of the current instance matches the timeline defined in
        # SS node.
        if int(switchover_info.get('timeline')) != db_state['timeline']:
            logging.warning(
                'Switchover node has timeline %s, but local is %s, ignoring ' 'switchover.',
                switchover_info.get('timeline'),
                db_state['timeline'],
            )
            return None

        # Last switchover was more than N sec ago
        last_failover_ts = self.zk.get(zk.LAST_FAILOVER_TIME_PATH, preproc=float)

        last_switchover_ts = self.zk.get(zk.LAST_SWITCHOVER_TIME_PATH, preproc=float)

        last_role_transition_ts = None
        if last_failover_ts is not None or last_switchover_ts is not None:
            last_role_transition_ts = max(filter(lambda x: x is not None, [last_switchover_ts, last_failover_ts]))

        alive_replics_number = len([i for i in db_state['replics_info'] if i['state'] == 'streaming'])

        ha_replics = self._get_ha_replics()
        if ha_replics is None:
            return None
        ha_replic_cnt = len(ha_replics)

        if not helpers.check_last_failover_time(last_role_transition_ts, self.config) and (
            alive_replics_number < ha_replic_cnt
        ):
            logging.warning(
                'Last role transition was %.1f seconds ago,'
                ' and alive host count less than HA hosts in zk (HA: %d, ZK: %d) ignoring switchover.',
                time.time() - last_role_transition_ts,
                ha_replic_cnt,
                alive_replics_number,
            )
            return None

        # Ensure there is no other failover in progress.
        failover_state = self.zk.get(zk.FAILOVER_INFO_PATH)
        if failover_state not in ('finished', None):
            logging.error('Switchover requested, but current failover state is %s.', failover_state)
            return None

        if self._get_switchover_candidate() is None:
            return False

        logging.info('Scheduled switchover checks passed OK.')
        return True

    def _do_master_switchover(self, db_state):
        """
        Perform steps required on scheduled switchover
        if current role is master
        """
        logging.warning('Starting scheduled switchover')
        self.zk.write(zk.SWITCHOVER_STATE_PATH, 'initiated')
        # Announce intention to perform switchover to the rest of the cluster.
        if not self.zk.write(zk.FAILOVER_INFO_PATH, 'switchover_initiated'):
            logging.error(f'unable to write failover state to zk ({zk.FAILOVER_INFO_PATH})')
            return False

        limit = self.config.getfloat('global', 'postgres_timeout')

        if not helpers.await_for(
            lambda: self.zk.get(zk.SWITCHOVER_STATE_PATH) == "candidate_found", limit, "switchover candidate found"
        ):
            return None

        # Deny user requests
        self.db.pgbouncer('stop')

        # Attempt to shut down local PG instance.
        # Failure is not critical.
        if self.db.stop_postgresql(timeout=limit) == 0:
            lsn = self._cmd_manager.get_control_parameter(db_state['pgdata'], "REDO location")
            self.zk.noexcept_write(zk.SWITCHOVER_LSN_PATH, lsn)
            if not self.zk.noexcept_write(zk.FAILOVER_INFO_PATH, 'switchover_master_shut'):
                logging.error(f'unable to write failover state to zk ({zk.FAILOVER_INFO_PATH})')
                return False
        else:
            logging.error('Unable to stop postgresql')
            return False

        # Release leader-lock.
        # Wait 5 secs for the actual release.
        self.zk.release_lock(lock_type=zk.MASTER_LOCK_PATH, wait=5)

        return True

    def _transition_master_switchover(self):
        """
        Wait for N seconds trying to find out new master,
        then transition to replica.
        If timeout passed and no one took the lock, rollback
        the procedure.
        """
        timeout = self.config.getfloat('global', 'postgres_timeout')
        if helpers.await_for(
            lambda: self.zk.get(zk.SWITCHOVER_STATE_PATH) is None, timeout, 'new master finished switchover'
        ):
            master = self.zk.get_current_lock_holder(zk.MASTER_LOCK_PATH)
            if master is not None:
                # From here switchover can be considered successful regardless of this host state
                self.zk.delete('%s/%s/op' % (zk.MEMBERS_PATH, helpers.get_hostname()))
                return self._attach_to_master(master, self.config.getfloat('replica', 'recovery_timeout'))
        # Mark switchover node as failure
        self.zk.write(zk.SWITCHOVER_STATE_PATH, 'replica_timed_out', need_lock=False)

    def _detect_replica_switchover(self):
        """
        Detect planned switchover condition.
        """

        if self.zk.get(zk.SWITCHOVER_STATE_PATH) is None:
            return False

        db_state = self.db.get_state()

        switchover_info = self.zk.get(zk.SWITCHOVER_MASTER_PATH, preproc=json.loads)
        if not switchover_info:
            return False

        # We check that switchover should happen from current timeline
        zk_tli = self.zk.get(zk.TIMELINE_INFO_PATH, preproc=int)
        if zk_tli != switchover_info['timeline']:
            return False

        # Scheduled switchover node with master (fqdn, tli) info exists.

        # The scheduled switchover was commenced by master:
        # 'switchover_initiated': the master is in the process
        # of shutting itself down

        # If there is an ability to do failover instead of switchover, than let's do it.
        autofailover = self.config.getboolean('global', 'autofailover')
        failover_state = self.zk.get(zk.FAILOVER_INFO_PATH)
        if failover_state not in ['switchover_initiated', 'switchover_master_shut'] and autofailover:
            return False

        # The node contains hostname of current instance
        switchover_master = switchover_info.get('hostname')
        if switchover_master is not None and switchover_master != db_state['master_fqdn']:
            logging.error('current master FQDN is not equal to ' 'hostname in switchover node, ignoring switchover')
            return False

        return True

    def _zk_alive_refresh(self, role, db_state, zk_state):
        self._replication_manager.drop_zk_fail_timestamp()
        if role is None:
            self.zk.release_lock(zk.get_host_alive_lock_path())
        else:
            self._update_single_node_status(role)
            if self.zk.get_current_lock_holder(zk.get_host_alive_lock_path()) is None:
                logging.warning("I don't hold my alive lock, let's acquire it")
                self.zk.try_acquire_lock(zk.get_host_alive_lock_path())

    def _zk_get_wal_receiver_info(self, host):
        return self.zk.get('{}/{}/wal_receiver'.format(zk.MEMBERS_PATH, host), preproc=json.loads)

    def _fix_hostname(self, replics_info):
        """
        MDB-6362: fill hostname in pg_stat_replication using pg_stat_wal_receiver
        """
        if not replics_info:
            return True
        try:
            # Fix hostnames only if isn't set
            if len(list(info for info in replics_info if info['client_hostname'] is None)) == 0:
                return True
            alive_hosts = zk.get_alive_hosts(self.zk)
            for host in alive_hosts:
                wal_receiver_info = self._zk_get_wal_receiver_info(host)
                if wal_receiver_info:
                    # We have only one walreceiver
                    conninfo = conninfo_to_dict(wal_receiver_info[0]['conninfo'])
                    application_name = conninfo.get('application_name')
                    for info in replics_info:
                        if (
                            application_name is not None
                            and info['application_name'] == application_name
                            and info['client_hostname'] is None
                        ):
                            logging.warning('Reverse DNS lookup for %s is failed', application_name)
                            info['client_hostname'] = host
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error('Unable to run fix_hostname: %s', line.rstrip())

    def is_op_destructive(self, op):
        return op in self.DESTRUCTIVE_OPERATIONS
