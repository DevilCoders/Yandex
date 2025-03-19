"""
Pg wrapper module. Postgres class defined here.
"""
# encoding: utf-8

from __future__ import absolute_import, print_function, unicode_literals

import contextlib
import json
import logging
import os
import re
import signal
import socket
import sys
import traceback
import time

import psycopg2
import psycopg2.errors
from psycopg2.sql import Identifier, SQL

from . import helpers

if sys.version_info < (3, 0):
    DEC2INT_TYPE = psycopg2.extensions.new_type(
        psycopg2.extensions.DECIMAL.values, b'DEC2INT', lambda value, curs: int(value) if value is not None else None
    )
else:
    DEC2INT_TYPE = psycopg2.extensions.new_type(
        psycopg2.extensions.DECIMAL.values, 'DEC2INT', lambda value, curs: int(value) if value is not None else None
    )

psycopg2.extensions.register_type(DEC2INT_TYPE)


def _get_names(cur):
    return [r[0].lower() for r in cur.description]


def _plain_format(cur):
    names = _get_names(cur)
    for row in cur.fetchall():
        yield dict(zip(names, tuple(row)))


class Postgres(object):
    """
    Postgres class
    """

    DISABLED_ARCHIVE_COMMAND = '/bin/false'

    def __init__(self, config, plugins, cmd_manager):
        self.config = config
        self._plugins = plugins
        self._cmd_manager = cmd_manager

        self.state = dict()

        self.conn_local = None
        self.role = None
        self.pgdata = None
        self.pg_version = None
        self._offline_detect_pgdata()
        self.reconnect()
        self.use_lwaldump = self.config.getboolean('global', 'use_lwaldump') or self.config.getboolean(
            'global', 'quorum_commit'
        )

    def _create_cursor(self):
        try:
            if self.conn_local:
                cursor = self.conn_local.cursor()
                cursor.execute('SELECT 1;')
                return cursor
            else:
                raise RuntimeError('Local conn is dead')
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.debug(line.rstrip())
            self.reconnect()

    def _exec_query(self, query, **kwargs):
        cur = self._create_cursor()
        if not cur:
            raise RuntimeError('Local conn is dead')
        cur.execute(query, kwargs)
        return cur

    def _get(self, query, **kwargs):
        with contextlib.closing(self._exec_query(query, **kwargs)) as cur:
            records = list(_plain_format(cur))
            return records

    def _exec_without_result(self, query):
        try:
            self._exec_query(query)
            return True
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return False

    def get_data_from_control_file(self, parameter, preproc=None, log=True):
        """
        Run pg_controldata and grep it's output
        """
        return self._cmd_manager.get_control_parameter(self.pgdata, parameter, preproc, log)

    def _local_conn_string_get_port(self):
        for param in self.config.get('global', 'local_conn_string').split():
            key, value = param.strip().split('=')
            if key == 'port':
                port = value
                break
        else:
            port = '5432'
        return port

    def _offline_detect_pgdata(self):
        """
        Try to find pgdata and version parameter from list_clusters command by port
        """
        try:
            state = {}
            need_port = self._local_conn_string_get_port()
            rows = self._cmd_manager.list_clusters()
            logging.debug(rows)
            for row in rows:
                if not row:
                    continue
                version, _, port, pgstate, _, pgdata, _ = row.split()
                if port != need_port:
                    continue
                if state.get('pg_version'):
                    logging.error('Found more than one cluster on %s port', need_port)
                    return
                self.pg_version = state['pg_version'] = version
                self.role = state['role'] = 'replica' if 'recovery' in pgstate else 'master'
                self.pgdata = state['pgdata'] = pgdata
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())

    @helpers.return_none_on_error
    def _get_replication_slots(self):
        res = self._exec_query('SELECT slot_name ' 'FROM pg_replication_slots;').fetchall()
        return [i[0] for i in res]

    def _create_replication_slot(self, slot_name):
        query = "SELECT pg_create_physical_replication_slot('%s', true);" % slot_name
        query_res = self._exec_without_result(query)
        if not query_res:
            return query_res
        return query_res

    def _drop_replication_slot(self, slot_name):
        query = "SELECT pg_drop_replication_slot('%s');" % slot_name
        return self._exec_without_result(query)

    def reconnect(self):
        """
        Reestablish connection with local postgresql
        """
        nonfatal_errors = [
            'OperationalError: FATAL:  the database system is starting up',
            'OperationalError: FATAL:  the database system is shutting down',
        ]
        try:
            if self.conn_local:
                self.conn_local.close()
            if not self.state.get('running', False):
                logging.error('PostgreSQL is dead. Unable to reconnect.')
                self.conn_local = None
                return
            self.conn_local = psycopg2.connect(self.config.get('global', 'local_conn_string'))
            self.conn_local.autocommit = True

            self.role = self.get_role()
            self.pg_version = self._get_pg_version()
            self.pgdata = self._get_pgdata_path()
        except psycopg2.OperationalError:
            logging.error('Could not connect to "%s".', self.config.get('global', 'local_conn_string'))
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
                if line.rstrip() in nonfatal_errors:
                    raise RuntimeError('PostgreSQL is running but not alive')

    def get_state(self):
        """
        Get current database state (if possible)
        """
        fname = '%s/.pgsync_db_state.cache' % self.config.get('global', 'working_dir')
        try:
            with open(fname, 'r') as fobj:
                prev = json.loads(fobj.read())
        except Exception:
            prev = None

        data = {'alive': False, 'prev_state': prev}
        try:
            try:
                data['running'] = self.is_alive()
                data['alive'] = self.is_alive()
            except Exception as err:
                if str(err) == 'PostgreSQL is running but not alive':
                    data['running'] = True
                else:
                    data['running'] = False
                data['alive'] = False
            # Explicitly update "running" to avoid dead loop
            self.state['running'] = data['running']

            if not data['alive']:
                raise RuntimeError('PostgreSQL is dead')
            data['role'] = self.get_role()
            self.role = data['role']
            data['pg_version'] = self._get_pg_version()
            data['pgdata'] = self._get_pgdata_path()
            data['opened'] = self.pgbouncer('status')
            data['timeline'] = self.get_data_from_control_file('Latest checkpoint.s TimeLineID', preproc=int, log=False)
            data['wal_receiver'] = self._get_wal_receiver_info()

            if data['role'] == 'master':
                data['replics_info'] = self.get_replics_info('master')
                data['replication_state'] = self.get_replication_state()
                data['sessions_ratio'] = self.get_sessions_ratio()
            elif data['role'] == 'replica':
                data['master_fqdn'] = self.recovery_conf('get_master')
                data['replics_info'] = self.get_replics_info('replica')

            #
            # We ask health of PostgreSQL one more time since it could die
            # while we were asking all other things here. It can lead to
            # unpredictable results.
            #
            data['alive'] = self.is_alive()
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())

        if data['alive']:
            try:
                with open(fname, 'w') as fobj:
                    save_data = data.copy()
                    del save_data['prev_state']
                    fobj.write(json.dumps(save_data))
            except IOError:
                logging.warning('Could not write cache file. Skipping it.')

        self.state = data
        return data

    def is_alive(self):
        """
        Check that postgresql is alive
        """
        try:
            # In order to check that postgresql is really alive
            # we need to check if service is running then
            # drop current connection and establish a new one
            if self.state.get('running', False):
                self.reconnect()
                res = self._exec_query('SELECT 42;').fetchone()
                if res[0] == 42:
                    return True
            else:
                self.state['running'] = self.get_postgresql_status() == 0
            return False
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.debug(line.rstrip())
                # TODO: Need Exception refactoring
                if line.rstrip() == 'RuntimeError: PostgreSQL is running but not alive':
                    raise RuntimeError('PostgreSQL is running but not alive')
            return False

    @helpers.return_none_on_error
    def get_role(self):
        """
        Get role of local postgresql (replica, master or None if dead)
        """
        res = self._exec_query('SELECT pg_is_in_recovery();')
        if res is None:
            return None
        elif res.fetchone()[0]:
            return 'replica'
        else:
            return 'master'

    @helpers.return_none_on_error
    def _get_pg_version(self):
        """
        Get local postgresql version
        """
        res = self._exec_query("SHOW server_version_num")
        return int(res.fetchone()[0])

    @helpers.return_none_on_error
    def _get_pgdata_path(self):
        """
        Get local pg_data
        """
        res = self._exec_query('SHOW data_directory;').fetchone()
        return res[0]

    @helpers.return_none_on_error
    def get_replics_info(self, role):
        """
        Get replicas from pg_stat_replication
        """
        version = self._get_pg_version()
        if version >= 100000:
            current_lsn = {'master': 'pg_current_wal_lsn()', 'replica': 'pg_last_wal_replay_lsn()'}
            wal_func = {
                'current_lsn': current_lsn[role],
                'diff_lsn': 'pg_wal_lsn_diff',
                'app_name': 'pg_receivewal',
                'sent_lsn': 'sent_lsn',
                'write_lsn': 'write_lsn',
                'replay_lsn': 'replay_lsn',
            }
            replay_lag = 'COALESCE(1000*EXTRACT(epoch from replay_lag), 0)::bigint AS replay_lag_msec,'
        else:
            current_lsn = {'master': 'pg_current_xlog_location()', 'replica': 'pg_last_xlog_replay_location()'}
            wal_func = {
                'current_lsn': current_lsn[role],
                'diff_lsn': 'pg_xlog_location_diff',
                'app_name': 'pg_receivexlog',
                'sent_lsn': 'sent_location',
                'write_lsn': 'sent_location',
                'replay_lsn': 'replay_location',
            }
            replay_lag = ''
        query = """SELECT pid, application_name,
                    client_hostname, client_addr, state,
                {current_lsn}
                    AS master_location,
                {diff_lsn}({current_lsn}, {sent_lsn})
                    AS sent_location_diff,
                {diff_lsn}({current_lsn}, {write_lsn})
                    AS write_location_diff,
                {diff_lsn}({current_lsn},
                    {replay_lsn})
                    AS replay_location_diff,
                {replay_lag}
                extract(epoch from backend_start)::bigint AS backend_start_ts,
                sync_state FROM pg_stat_replication
                WHERE application_name != 'pg_basebackup'
                AND application_name != '{app_name}'
                AND state = 'streaming'""".format(
            current_lsn=wal_func['current_lsn'],
            diff_lsn=wal_func['diff_lsn'],
            app_name=wal_func['app_name'],
            sent_lsn=wal_func['sent_lsn'],
            write_lsn=wal_func['write_lsn'],
            replay_lag=replay_lag,
            replay_lsn=wal_func['replay_lsn'],
        )
        return self._get(query)

    @helpers.return_none_on_error
    def _get_wal_receiver_info(self):
        """
        Get wal_receiver info from pg_stat_wal_receiver
        """
        query = """SELECT pid, status, slot_name,
                   conninfo FROM pg_stat_wal_receiver"""
        return self._get(query)

    @helpers.return_none_on_error
    def get_replication_state(self):
        """
        Get replication type (sync/async)
        """
        res = self._exec_query('SHOW synchronous_standby_names;').fetchone()
        res = ('async', None) if res[0] == '' else ('sync', res[0])
        return res

    @helpers.return_none_on_error
    def get_sessions_ratio(self):
        """
        Get ratio of active sessions/max sessions (in percents)
        """
        cur = self._exec_query("SELECT count(*) FROM pg_stat_activity WHERE state!='idle';")
        cur = cur.fetchone()[0]
        max_sessions = self._exec_query('SHOW max_connections;').fetchone()[0]
        return (cur / int(max_sessions)) * 100

    def _execute_versioned_query(self, old_version_query, new_version_query):
        version = self._get_pg_version()
        if version >= 100000:
            return self._exec_query(new_version_query)
        else:
            return self._exec_query(old_version_query)

    @helpers.return_none_on_error
    def lwaldump(self):
        """Protected from kill -9 postgres"""
        query = """SELECT pg_wal_lsn_diff(
                lwaldump(),
                '0/00000000')::bigint"""
        res = self._exec_query(query).fetchone()
        return res[0]

    @helpers.return_none_on_error
    def get_wal_receive_lsn(self):
        if self.use_lwaldump:
            return self.lwaldump()
        old_query = """SELECT pg_xlog_location_diff(
                    pg_last_xlog_receive_location(),
                    '0/00000000')::bigint"""
        new_query = """SELECT pg_wal_lsn_diff(
                    pg_last_wal_receive_lsn(),
                    '0/00000000')::bigint"""
        res = self._execute_versioned_query(old_query, new_query).fetchone()
        return res[0]

    def check_walsender(self, replics_info, holder_fqdn):
        """
        Check walsender in sync state and sync holder is same
        """
        if not replics_info:
            return True
        holder_app_name = helpers.app_name_from_fqdn(holder_fqdn)
        for replica in replics_info:
            try:
                if replica['sync_state'] == 'sync' and replica['application_name'] != holder_app_name:
                    logging.warning(
                        'It seems sync replica and ' 'sync replica holder is different. ' 'Killing walsender.'
                    )
                    os.kill(replica['pid'], signal.SIGTERM)
                    break
            except Exception as exc:
                logging.error('Check walsender error: %s', repr(exc))
        return True

    def check_walreceiver(self, replics_info):
        """
        Check if walreceiver is running using pg_stat_wal_receiver view
        """
        try:
            cur = self._exec_query('SELECT pid FROM pg_stat_wal_receiver WHERE status = \'streaming\'')
        except psycopg2.errors.UndefinedTable as exc:
            # We are not failing on exception here
            # for compatibility with pg < 9.6
            logging.debug('Unable to get wal receiver state: %s', repr(exc))
            return True
        except Exception as exc:
            logging.error('Unable to get wal receiver state: %s', repr(exc))
            return False
        rows = cur.fetchall()
        if not rows:
            # Wal receiver is not running so we fail
            return False
        pid = rows[0][0]
        for replica in replics_info:
            try:
                if (
                    replica['client_hostname'] is None
                    and socket.gethostbyaddr(replica['client_addr'])[0] == helpers.get_hostname()
                ):
                    logging.warning(
                        'It seems that we are '
                        'streaming but client_hostname '
                        'on master is empty. '
                        'Killing walreceiver.'
                    )
                    os.kill(pid, signal.SIGTERM)
                    return False
            except Exception as exc:
                logging.error('Check walreceiver error: %s', repr(exc))
                return False
        return True

    def is_ready_for_pg_rewind(self):
        """
        Check if pg_rewind could be used on local postgresql
        """
        res = self.get_data_from_control_file('Data page checksum version', preproc=int)
        if res:
            logging.info("Checksums are enabled, host is ready for pg_rewind.")
            return True

        res = self.get_data_from_control_file('wal_log_hints setting')
        if res == 'on':
            logging.info("Checksums are disabled but wal_log_hints = on, " "host is ready for pg_rewind.")
            return True

        logging.error("Checksums or wal_log_hints should be enabled for " "pg_rewind to work properly.")
        return False

    @helpers.return_none_on_error
    def get_replay_diff(self, diff_from='0/00000000'):
        new_query = f"""SELECT pg_wal_lsn_diff(
                    pg_last_wal_replay_lsn(),
                    '{diff_from}')::bigint"""
        old_query = f"""SELECT pg_xlog_location_diff(
                    pg_last_xlog_replay_location(),
                    '{diff_from}')::bigint"""
        res = self._execute_versioned_query(old_query, new_query).fetchone()
        return res[0]

    def recovery_conf(self, action, master_host=None):
        """
        Perform recovery conf action (create, remove, get_master)
        """
        recovery_filepath = os.path.join(self.pgdata, self.config.get('global', 'recovery_conf_rel_path'))

        if action == 'create':
            self._plugins.run('before_populate_recovery_conf', master_host)
            res = self._cmd_manager.generate_recovery_conf(recovery_filepath, master_host)
            self._plugins.run('after_populate_recovery_conf', master_host)
            return res
        elif action == 'remove':
            cmd = 'rm -f ' + recovery_filepath
            return helpers.subprocess_call(cmd)
        else:
            if os.path.exists(recovery_filepath):
                with open(recovery_filepath, 'r') as recovery_file:
                    for i in recovery_file.read().split('\n'):
                        if 'primary_conninfo' in i:
                            master = re.search(r'host=([\w\-\._]*)', i).group(0).split('=')[-1]
                            return master
            return None

    def promote(self):
        """
        Make local postgresql master
        """
        # TODO : potential split brain here in this case:
        # 1. We requested for switchover
        # 2. Host A was chosen to become a new master
        # 3. Host A promote took too much time, so old master decided to rollback switchover
        # 4. After switchover rollback and old master returned back as a master promote finished
        # 5. In the end we have old master with open pgbouncer and host A as a master with open pgbouncer.
        self._plugins.run('before_promote', self.conn_local, self.config)

        # We need to stop archiving WAL and resume after promote
        # to prevent wrong history file in archive in case of failure
        if not self.stop_archiving_wal():
            logging.error('Could not stop archiving WAL')
            return False

        # We need to resume replaying WAL before promote
        self.pg_wal_replay_resume()

        promoted = self._cmd_manager.promote(self.pgdata) == 0
        if promoted:
            if not self.resume_archiving_wal():
                logging.error('Could not resume archiving WAL')
            if self._wait_for_master_role():
                self._plugins.run('after_promote', self.conn_local, self.config)
        return promoted

    def _wait_for_master_role(self):
        """
        Wait until promotion succeeds
        """
        sleep_time = self.config.getfloat('global', 'iteration_timeout')
        role = self.get_role()
        while role != 'master':
            logging.debug('Our role should be master but we are now "%s".', role)
            if role is None:
                return False
            logging.info('Waiting %.1f second(s) to become master.', sleep_time)
            time.sleep(sleep_time)
            role = self.get_role()
        return True

    def pgbouncer(self, action):
        """
        Start/stop/status pgbouncer wrapper
        """
        if action == 'stop':
            if bool(self._cmd_manager.get_pgbouncer_status()):
                return True
            self._plugins.run('before_close_from_load')
            res = self._cmd_manager.stop_pgbouncer()
            after = 'after_close_from_load'
        elif action == 'status':
            standalone_pgbouncer = self.config.getboolean('global', 'standalone_pgbouncer')
            pgbouncer_addr = self.config.get('global', 'pgbouncer_addr')
            pgbouncer_port = self.config.get('global', 'pgbouncer_port')
            pgbouncer_conn_timeout = self.config.getfloat('global', 'pgbouncer_conn_timeout')
            if standalone_pgbouncer:
                try:
                    socket.create_connection((pgbouncer_addr, pgbouncer_port), pgbouncer_conn_timeout)
                    return True
                except socket.error:
                    return not bool(self._cmd_manager.get_pgbouncer_status())
            else:
                return not bool(self._cmd_manager.get_pgbouncer_status())
        elif action == 'start':
            if not bool(self._cmd_manager.get_pgbouncer_status()):
                return True
            self._plugins.run('before_open_for_load')
            res = self._cmd_manager.start_pgbouncer()
            after = 'after_open_for_load'
        else:
            raise RuntimeError('Unknown pgbouncer action: %s' % action)
        if res == 0:
            self._plugins.run(after)
            return True
        return False

    def do_rewind(self, master_host):
        """
        Run pg_rewind on localhost against master_host
        """
        if self.config.getboolean('global', 'use_replication_slots'):
            #
            # We should move pg_replslot directory somewhere before rewind
            # and move it back after it since pg_rewind doesn't do it.
            #
            try:
                helpers.backup_dir('%s/pg_replslot' % self.pgdata, '/tmp/pgsync_replslots_backup')
            except Exception:
                logging.warning('Could not backup replication slots before ' 'rewinding. Skipping it.')
        res = self._cmd_manager.rewind(self.pgdata, master_host)

        if self.config.getboolean('global', 'use_replication_slots') and res == 0:
            if os.path.exists('/tmp/pgsync_replslots_backup'):
                try:
                    helpers.backup_dir('/tmp/pgsync_replslots_backup', '%s/pg_replslot' % self.pgdata)
                except Exception:
                    logging.warning('Could not restore replication slots after' ' rewinding. Skipping it.')
        return res

    def change_replication_to_async(self):
        return self._change_replication_type('')

    def change_replication_to_sync_host(self, host_fqdn):
        return self._change_replication_type(helpers.app_name_from_fqdn(host_fqdn))

    def change_replication_to_quorum(self, replica_list):
        quorum_size = (len(replica_list) + 1) // 2
        replica_list = list(map(helpers.app_name_from_fqdn, replica_list))
        return self._change_replication_type(f"ANY {quorum_size}({','.join(replica_list)})")

    def _alter_system_set_param(self, param, value=None, reset=False):
        try:
            if reset:
                logging.debug(f'Resetting {param} with ALTER SYSTEM')
                query = SQL("ALTER SYSTEM RESET {param}").format(param=Identifier(param))
                self._exec_query(query.as_string(self.conn_local))
            else:
                logging.debug(f'Setting {param} to {value} with ALTER SYSTEM')
                query = SQL("ALTER SYSTEM SET {param} TO %(value)s").format(param=Identifier(param))
                self._exec_query(query.as_string(self.conn_local), value=value)
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return False
        # reload returns subprocess exit code, we need to convert it to boolean with inverse
        return not bool(self._cmd_manager.reload_postgresql(self.pgdata))

    def _change_replication_type(self, synchronous_standby_names):
        return self._alter_system_set_param('synchronous_standby_names', synchronous_standby_names)

    def ensure_archiving_wal(self):
        cursor = self._exec_query("SHOW archive_command")
        (archive_command,) = cursor.fetchone()
        if archive_command == self.DISABLED_ARCHIVE_COMMAND:
            logging.info('Archive command was disabled, enabling it')
            self.resume_archiving_wal()
        config = self._get_postgresql_auto_conf()
        if config.get('archive_command') == self.DISABLED_ARCHIVE_COMMAND:
            logging.info('Archive command was disabled in postgresql.auto.conf, resetting it')
            self.resume_archiving_wal()

    def stop_archiving_wal(self):
        return self._alter_system_set_param('archive_command', self.DISABLED_ARCHIVE_COMMAND)

    def resume_archiving_wal(self):
        return self._alter_system_set_param('archive_command', reset=True)

    def stop_archiving_wal_stopped(self):
        return self._alter_system_stopped('archive_command', self.DISABLED_ARCHIVE_COMMAND)

    def _get_postgresql_auto_conf(self):
        config = {}
        current_file = os.path.join(self.pgdata, 'postgresql.auto.conf')
        with open(current_file, 'r') as fobj:
            for line in fobj:
                if line.lstrip().startswith('#'):
                    continue
                key, value = line.rstrip('\n').split('=', maxsplit=1)
                config[key.strip()] = value.lstrip().lstrip('\'').rstrip('\'')
        return config

    #
    # We do it with writing to file and not with ALTER SYSTEM command since
    # PostgreSQL is stopped when this method is called.
    # We are not afraid of future rewriting postgresql.auto.conf with ALTER
    # SYSTEM command since this change is temporary.
    #
    def _alter_system_stopped(self, param, set_value):
        """
        Set param to value while PostgreSQL is stopped.
        Method should be called only with stopped PostgreSQL.
        """
        try:
            logging.debug(f'Setting {param} to {set_value} in postgresql.auto.conf')
            config = self._get_postgresql_auto_conf()
            current_file = os.path.join(self.pgdata, 'postgresql.auto.conf')
            new_file = os.path.join(self.pgdata, 'postgresql.auto.conf.new')
            old_value = config.get(param)
            if old_value == set_value:
                logging.debug(f'Param {param} already has value {set_value} in postgresql.auto.conf')
                return True
            logging.debug(f'Changing {param} from {old_value} to {set_value} in postgresql.auto.conf')
            config[param] = set_value
            with open(new_file, 'w') as fobj:
                fobj.write('# Do not edit this file manually!\n')
                fobj.write('# It will be overwritten by the ALTER SYSTEM command.\n')
                for key, value in config.items():
                    fobj.write(f'{key} = \'{value}\'\n')
            os.replace(new_file, current_file)
            return True
        except Exception:
            for line in traceback.format_exc().split('\n'):
                logging.error(line.rstrip())
            return False

    def checkpoint(self, query=None):
        """
        Perform checkpoint
        """
        logging.warning('Initiating checkpoint')
        if not query:
            query = 'CHECKPOINT'
        return self._exec_without_result(query)

    def start_postgresql(self, timeout=60):
        """
        Start PG server on current host
        """
        return self._cmd_manager.start_postgresql(timeout, self.pgdata)

    def get_postgresql_status(self):
        """
        Returns PG status on current host
        """
        return self._cmd_manager.get_postgresql_status(self.pgdata)

    def stop_postgresql(self, timeout=60):
        """
        Stop PG server on current host

        If synchronous replication is ON, but sync replica is dead, then we aren't able to stop PG.
        """
        try:
            self.change_replication_to_async()  # TODO : it can lead to data loss
        except Exception:
            logging.warning('Could not disable synchronous replication.')
            for line in traceback.format_exc().split('\n'):
                logging.warning(line.rstrip())
        return self._cmd_manager.stop_postgresql(timeout, self.pgdata)

    def replication_slots(self, action, slots):
        """
        Perform replication slots action (create/drop)
        """
        current = self._get_replication_slots()
        for slot in slots:
            if action == 'create':
                if current and slot in current:
                    logging.warning('Slot %s already exists.', slot)
                    continue
                if not self._create_replication_slot(slot):
                    return False
            else:
                if current and slot not in current:
                    logging.warning('Slot %s does not exist.', slot)
                    continue
                if not self._drop_replication_slot(slot):
                    return False
        return True

    def is_replaying_wal(self, check_time):
        prev_replay_diff = self.get_replay_diff()
        time.sleep(check_time)
        replay_diff = self.get_replay_diff()
        return prev_replay_diff < replay_diff

    def pg_wal_replay_pause(self):
        self._pg_wal_replay("pause")

    def pg_wal_replay_resume(self):
        self._pg_wal_replay("resume")

    def is_wal_replay_paused(self):
        cur = self._execute_versioned_query(
            'SELECT pg_is_xlog_replay_paused();',
            'SELECT pg_is_wal_replay_paused();',
        )
        (paused,) = cur.fetchone()
        return paused

    def ensure_replaying_wal(self):
        if self.is_wal_replay_paused():
            logging.warning('WAL replay is paused')
            self.pg_wal_replay_resume()

    def terminate_backend(self, pid):
        """
        Send sigterm to backend by pid
        """
        # Note that pid could be already dead by this moment
        # So we do not check result
        self._exec_without_result(f'SELECT pg_terminate_backend({pid})')

    def _pg_wal_replay(self, pause_or_resume):
        logging.debug('WAL replay: %s', pause_or_resume)
        self._execute_versioned_query(
            f'SELECT pg_xlog_replay_{pause_or_resume}();',
            f'SELECT pg_wal_replay_{pause_or_resume}();',
        )

    def check_extension_installed(self, name):
        cur = self._exec_query(f"SELECT * FROM pg_extension WHERE extname = '{name}';")
        result = cur.fetchall()
        return len(result) == 1

    def reload(self):
        return not bool(self._cmd_manager.reload_postgresql(self.pgdata))
