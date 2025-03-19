#!/usr/bin/env python
# coding: utf-8

from __future__ import print_function

import hashlib
import json
import logging
import os.path
import subprocess
import threading
import time
from argparse import ArgumentParser
from collections import defaultdict
from datetime import datetime

import psycopg2
import psycopg2.extensions

log = logging.getLogger(__name__)
LOG_SUPPRESS_OPTIONS = (
    '-c log_statement=none -c log_min_messages=panic '
    '-c log_min_error_statement=panic -c log_min_duration_statement=-1'
)
UTILITY_OPTION = ' -c gp_session_role=utility'

class Greenplum(object):
    """
    Greenplum helpers
    """
    restore_port = 3432
    alive_port = 5432

    restore_env = {"PGPASSFILE": "/home/gpadmin/.pgpass_restore"}
    alive_env = {"PGPASSFILE": "/home/gpadmin/.pgpass"}

    recovery_timeout = 120 * 60  # 120 minutes

    def __init__(self, gp_bin, gp_master_data, segment_prefix, gp_version, gp_restore_cfg):
        self.bin = gp_bin
        self.master_data = gp_master_data
        self.version = gp_version
        self.restore_config_path = gp_restore_cfg
        self.segment_prefix = segment_prefix

    def make_gp_cmd(self, cmd, *cmd_args):
        return [os.path.join(self.bin, cmd)] + list(cmd_args)

    @classmethod
    def make_restore_dsn(cls, utility_mode=True):
        options = LOG_SUPPRESS_OPTIONS
        if utility_mode:
            options += UTILITY_OPTION

        return "dbname=postgres port=%d options='%s'" % (cls.restore_port, options)

    def path_to_gp_master_file(self, file_path):
        return os.path.join(self.master_data, 'master', self.segment_prefix + '-1', file_path)


def run_command(cmd_args, env=None):
    if env is None:
        env = dict()
    log.info('Execute %s', cmd_args)
    cmd = subprocess.Popen(
        cmd_args,
        stderr=subprocess.PIPE,
        stdout=subprocess.PIPE,
        env=dict(os.environ, **env)
    )
    out, err = cmd.communicate()

    log.info(
        "cmd: '{}': code: {}, out: '{}', err: '{}'".format(cmd_args, cmd.returncode, out, err)
    )

    return cmd.returncode == 0


def start_master_only(gp):
    cmd_args = gp.make_gp_cmd(
        'gpstart',
        # master only
        '-m',
        # timeout
        '-t',
        str(gp.recovery_timeout),
        # non-interactive mode
        '-a',
    )
    return run_command(cmd_args, gp.restore_env)


def stop_master_only(gp):
    cmd_args = gp.make_gp_cmd(
        'gpstop',
        # master only
        '-m',
        # timeout
        '-t',
        str(gp.recovery_timeout),
        # non-interactive mode
        '-a',
    )
    return run_command(cmd_args, gp.restore_env)


def start_cluster(gp, use_restore_env=True):
    cmd_args = gp.make_gp_cmd(
        'gpstart',
        # timeout
        '-t',
        str(gp.recovery_timeout),
        # non-interactive mode
        '-a',
    )
    if use_restore_env:
        env = gp.restore_env
    else:
        env = gp.alive_env

    return run_command(cmd_args, env)


def stop_cluster(gp):
    cmd_args = gp.make_gp_cmd(
        'gpstop',
        # timeout
        '-t',
        str(gp.recovery_timeout),
        # non-interactive mode
        '-a',
    )
    return run_command(cmd_args, gp.alive_env)


def recover_segments(gp):
    cmd_args = gp.make_gp_cmd(
        'gprecoverseg',
        # do the full copy (incremental recovery is not possible since there is no data on mirrors)
        '-F',
        # non-interactive mode
        '-a',
    )
    return run_command(cmd_args, gp.alive_env)


def set_master_port(gp, port):
    postgresql_conf = gp.path_to_gp_master_file('postgresql.conf')
    with open(postgresql_conf, 'a') as c:
        # append 'port=xxx' to the end of the config file
        c.write('port=%s\n' % port)


def is_greenplum_ready(gp, old_gpadmin_password=None):
    dsn = gp.make_restore_dsn()
    try:
        conn = psycopg2.connect(dsn)
        log.debug('is_greenplum_ready: Connected to greenplum')
    except psycopg2.OperationalError as exc:
        if old_gpadmin_password is not None:
            try:
                conn = psycopg2.connect(dsn + " password=" + old_gpadmin_password)
                log.debug('is_greenplum_ready: Connected to greenplum with old password')
            except psycopg2.OperationalError as exc:
                log.debug('is_greenplum_ready: Can\'t connect to greenplum: %s', exc)
                return False
        else:
            log.debug('is_greenplum_ready: Can\'t connect to greenplum: %s', exc)
            return False

    conn.autocommit = True
    cur = conn.cursor()
    cur.execute('SELECT pg_is_in_recovery()')
    is_in_recovery = cur.fetchone()[0]

    log.debug('is_in_recovery: %r', is_in_recovery)
    conn.close()
    return not is_in_recovery


CHANGER_PWD_TMPL = "ALTER ROLE {role} WITH ENCRYPTED PASSWORD '{password}'"
UPDATE_SEG_CONFIG_TMPL = "UPDATE gp_segment_configuration SET hostname='{hostname}', address='{address}', port={port}, datadir='{datadir}', status='{status}', mode='{mode}' WHERE content={content_id} and role='{role}'"


def init_logging():
    root_logger = logging.getLogger()
    formatter = logging.Formatter('%(asctime)s - %(funcName)s [%(levelname)s]: %(message)s')
    root_logger.setLevel(logging.DEBUG)

    to_file = logging.FileHandler('/var/log/greenplum/restore-from-backup.log')
    to_file.setFormatter(formatter)
    to_file.setLevel(logging.DEBUG)
    root_logger.addHandler(to_file)

    to_out = logging.StreamHandler()
    to_out.setFormatter(formatter)
    to_out.setLevel(logging.WARNING)
    root_logger.addHandler(to_out)


def update_gp_segment_configuration(gp, conn):
    conn.autocommit = True
    cursor = conn.cursor()
    # Allow system table modifications
    cursor.execute("SET allow_system_table_mods=TRUE;")

    with open(gp.restore_config_path) as restore_cfg:
        cfg = json.load(restore_cfg)
        primaries = ('p', 'u', cfg['segments'])
        mirrors = ('m', 'd', cfg['mirrors'])

        # drop the standby master entry
        query = "DELETE FROM gp_segment_configuration WHERE content=-1 AND role='m';"
        log.debug("Updating gp_segment_configuration entry, query %s" % query)
        cursor.execute(query)

        for role, status, config in [primaries, mirrors]:
            for content_id, segment in config.items():
                if role == 'm' and content_id == "-1":
                    # do not create entry for standby master
                    continue

                query = UPDATE_SEG_CONFIG_TMPL.format(
                    hostname=segment['hostname'],
                    address=segment['hostname'],
                    port=segment['port'],
                    datadir=segment['data_dir'],
                    content_id=content_id,
                    role=role,
                    status=status,
                    mode='n')  # non-synced mode
                log.debug("Updating gp_segment_configuration entry, query %s" % query)
                cursor.execute(query)


class RestoreActions(object):
    """
    restore action
    """

    log_restoring_seconds = 180

    def __init__(self, state_dir, new_gpadmin_password, old_gpadmin_password, new_monitor_password, gp):
        self.state_dir = state_dir
        self.new_gpadmin_password = new_gpadmin_password
        self.old_gpadmin_password = old_gpadmin_password
        self.new_monitor_password = new_monitor_password
        self.gp = gp

    def set_master_restore_port(self):
        set_master_port(self.gp, self.gp.restore_port)

    def unset_master_restore_port(self):
        set_master_port(self.gp, self.gp.alive_port)

    def start_master(self):
        """
        Start greenplum in master-only mode
        """
        if not start_master_only(self.gp):
            raise RuntimeError("Failed to start up the master for configuration change")

    def start_cluster_restore(self):
        """
        Start greenplum cluster in restore mode
        """
        log.debug('Starting up the cluster restore')
        if not start_cluster(self.gp, use_restore_env=True):
            raise RuntimeError("Failed to start up the cluster for restore")

    def start_cluster_alive(self):
        """
        Start greenplum cluster in alive mode
        """
        log.debug('Starting up the cluster after restore')
        if not start_cluster(self.gp, use_restore_env=False):
            raise RuntimeError("Failed to start up the cluster after restore")

    def update_configuration(self):
        dsn = self.gp.make_restore_dsn()
        try:
            conn = psycopg2.connect(dsn)
            conn.autocommit = True
            log.debug('Connected to Greenplum')
        except psycopg2.OperationalError as exc:
            log.debug('Can\'t connect to greenplum: %s', exc)
            raise RuntimeError('Update gp_segment_configuration: failed to connect to greenplum: %s' % exc)

        log.debug('Update gp_segment_configuration: connected to greenplum')
        update_gp_segment_configuration(self.gp, conn)
        conn.close()

    def stop_master(self):
        if not stop_master_only(self.gp):
            raise RuntimeError("Failed to stop the Greenplum master")

    def stop_cluster(self):
        if not stop_cluster(self.gp):
            raise RuntimeError("Failed to stop Greenplum cluster")

    def recover_mirrors(self):
        if not recover_segments(self.gp):
            raise RuntimeError("Failed to recover mirrors")

    def change_gpadmin_password(self):
        # we don't want utility mode here because we want to change password on all segments,
        # not only on the master
        dsn = self.gp.make_restore_dsn(utility_mode=False)

        conn = psycopg2.connect(dsn + " password=" + self.old_gpadmin_password)
        conn.autocommit = True
        cur = conn.cursor()

        if self.new_gpadmin_password:
            log.info("Changing the gpadmin password")
            cur.execute(CHANGER_PWD_TMPL.format(role="gpadmin", password=self.new_gpadmin_password))
        else:
            log.info("Not changing the gpadmin password")

        if self.new_monitor_password:
            log.info("Changing the monitor password")
            cur.execute(CHANGER_PWD_TMPL.format(role="monitor", password=self.new_monitor_password))
        else:
            log.info("Not changing the monitor password")
        conn.close()

    def _path_to(self, name):
        return os.path.join(self.state_dir, name.__name__)

    def _state_passed(self, name):
        return os.path.exists(self._path_to(name))

    def _mark_state_passed(self, name):
        with open(self._path_to(name), 'w') as fd:
            fd.write('done at %s' % datetime.now())

    def remove_recovery_conf(self):
        """
        In case of highstate retry we will have master that fail on restart
        """
        recovery_conf = self.gp.path_to_gp_master_file('recovery.conf')
        if os.path.exists(recovery_conf):
            log.info("recovery.conf %r exists. Removing it.", recovery_conf)
            os.unlink(recovery_conf)

    def __call__(self):
        for func in [
            self.set_master_restore_port,
            self.start_master,
            self.update_configuration,
            self.stop_master,
            self.start_cluster_restore,
            self.change_gpadmin_password,
            self.recover_mirrors,
            self.stop_cluster,
            self.unset_master_restore_port,
            self.start_cluster_alive,
        ]:
            if self._state_passed(func):
                log.info('Skip state %r, because it is completed', func)
                continue
            log.info('Start step %r', func)
            func()
            self._mark_state_passed(func)
            log.info('Step %r finished', func)
        # Always remove recovery.conf
        # If it left:
        # - next Greenplum restart will fail,
        #   because Greenplum will find it and will try to start point-in-time recovery
        # - adding host to that cluster will fail,
        #   because basebackup transfers it from master
        log.info("Remove recovery.conf")
        self.remove_recovery_conf()


def main():
    parser = ArgumentParser()
    parser.add_argument(
        '--recovery-state',
        metavar='PATH',
        help='path to recovery state dir',
    )
    parser.add_argument(
        '--gp-master-data',
        metavar='PATH',
        help='path to Greenplum master',
        required=True,
    )
    parser.add_argument(
        '--gp-segment-prefix',
        help='Greenplum segments prefix',
        default='gpseg',
        required=False,
    )
    parser.add_argument(
        '--gp-bin',
        metavar='PATH',
        help='path to Greenplum utils',
        required=True,
    )
    parser.add_argument(
        '--gp-version',
        help='Greenplum major version',
        type=int,
        required=True,
    )
    parser.add_argument(
        '--new-gpadmin-password',
        help='new gpadmin user password',
        required=False,
    )
    parser.add_argument(
        '--new-monitor-password',
        help='new monitor user password',
        required=False,
    )
    parser.add_argument(
        '--old-gpadmin-password',
        help='old gpadmin user password',
        required=False,
    )
    parser.add_argument(
        '--restore-config',
        metavar='PATH',
        help='Path to the WAL-G restore config',
        required=True,
    )
    init_logging()
    args = parser.parse_args()

    restore = RestoreActions(
        state_dir=args.recovery_state,
        new_gpadmin_password=args.new_gpadmin_password,
        old_gpadmin_password=args.old_gpadmin_password,
        new_monitor_password=args.new_monitor_password,
        gp=Greenplum(
            gp_bin=args.gp_bin,
            gp_master_data=args.gp_master_data,
            segment_prefix=args.gp_segment_prefix,
            gp_version=args.gp_version,
            gp_restore_cfg=args.restore_config,
        ),
    )

    log.info('Start restore')
    try:
        restore()
    except Exception:
        log.exception('Got unexpected exception')
        raise


if __name__ == '__main__':
    main()

