{%- from "components/greenplum/map.jinja" import gpdbvars with context -%}
# !/usr/bin/env python
# -*- coding: utf-8 -*-

import psycopg2
import os.path
import json
import logging
import traceback
import sys
from datetime import datetime
from argparse import ArgumentParser
from gp_restore_from_backup import run_command
from gp_host_control import switch_master, disable_greenplum_service
from gp_autorecovery import AUTORECOVERY_DISABLED_FLAG
from enum import Enum
from os import path

class IsInRecoveryError(RuntimeError):
    pass

class IsStartingUpError(RuntimeError):
    pass

class IsShuttingDownError(RuntimeError):
    pass

class MasterStatus(str, Enum):
    down = 'down'  # no Greenplum process running
    dead = 'dead'  # Greenplum process running but database is dead
    startup = 'startup'  # Greenplum process running and starting up
    shutdown = 'shutdown'  # Greenplum process running and shutting down
    alive = 'alive'  # Greenplum process running and receiving connections
    in_recovery = 'in_recovery'  # Greenplum process running in recovery mode

LOG_FILE = '{{ gpdbvars.gplog }}/gp_master_autorecovery.log'
STATE_FILE = '/tmp/gp_master_autorecovery.state'

log = logging.getLogger(__name__)


def init_logging():
    root = logging.getLogger()
    root.setLevel(logging.DEBUG)
    fh = logging.FileHandler(LOG_FILE)
    fh.setFormatter(logging.Formatter('%(asctime)s - %(funcName)s [%(levelname)s]: %(message)s'))
    root.addHandler(fh)


class Greenplum(object):
    """
    Greenplum master recovery helper
    """
    port = 5432

    def __init__(self, gp_bin, gp_master_data, gp_master_fqdns, gp_local_fqdn):
        self.bin = gp_bin
        self.master_data = gp_master_data
        self.conn_local = None
        self.state = dict()
        self.local_host = gp_local_fqdn
        self.remote_host = next(fqdn for fqdn in gp_master_fqdns if fqdn != gp_local_fqdn)

    def make_gp_cmd(self, cmd, *cmd_args):
        return [os.path.join(self.bin, cmd)] + list(cmd_args)

    def make_dsn(self):
        return "dbname=postgres port=%d options='-c gp_session_role=utility'" % self.port

    def gp_is_down(self):
        cmd_args = self.make_gp_cmd(
            'pg_ctl',
            'status',
            '--pgdata',
            self.master_data,
        )
        return not run_command(cmd_args)

    def reconnect(self):
        """
        Reestablish connection with local greenplum
        """

        known_errors = {
            'OperationalError: FATAL:  the database system is starting up': IsStartingUpError,
            'OperationalError: FATAL:  the database system is shutting down': IsShuttingDownError,
            'OperationalError: FATAL:  the database system is in recovery mode': IsInRecoveryError
        }

        try:
            if self.conn_local:
                self.conn_local.close()
            self.conn_local = psycopg2.connect(self.make_dsn())
            self.conn_local.autocommit = True
        except psycopg2.OperationalError:
            log.error('Could not connect to "%s".', self.make_dsn())
            for line in traceback.format_exc().split('\n'):
                log.error(line.rstrip())
                error = known_errors.get(line.rstrip(), None)
                if error is not None:
                    raise error()
            raise RuntimeError('Failed to connect to Greenplum Master')


    def get_state(self):
        """
        Get current database state (if possible)
        """
        try:
            with open(STATE_FILE, 'r') as sf:
                prev = json.loads(sf.read())
        except Exception:
            prev = None

        status = self.check_master_status()
        data = {'status': status, 'ts': datetime.now().strftime("%Y-%m-%d %H:%M:%S"), 'prev_state': prev}

        if status != MasterStatus.alive:
            log.warning("Master is not alive, status: %s", status)

        try:
            with open(STATE_FILE, 'w') as sf:
                save_data = data.copy()
                del save_data['prev_state']
                sf.write(json.dumps(save_data))
        except IOError:
            log.warning('Could not write cache file. Skipping it.')

        self.state = data
        return data


    def check_master_status(self):
        """
        Check that greenplum is alive.
        Returns MasterStatus
        """
        if self.gp_is_down():
            return MasterStatus.down

        # Master process exists, check its state
        try:
            self.reconnect()
            cursor = self.conn_local.cursor()
            cursor.execute('SELECT 42;')
            res = cursor.fetchone()
            if res[0] == 42:
                return MasterStatus.alive

            return MasterStatus.dead
        except IsStartingUpError:
            return MasterStatus.startup
        except IsShuttingDownError:
            return MasterStatus.shutdown
        except IsInRecoveryError:
            return MasterStatus.in_recovery
        except Exception:
            log.error(traceback.format_exc())
            return MasterStatus.dead


    def activate_standby(self):
        disable_greenplum_service()
        switch_master(self.local_host, self.remote_host)


    def run(self):
        state = self.get_state()
        log.info('Greenplum state is: %s', state)
        prev_state = state.get('prev_state', None)
        if prev_state and state['status'] == prev_state['status'] == MasterStatus.down:
            log.info('Master seems to be down. Now will try to activate the standby.')
            self.activate_standby()
            return

        log.info('Not doing anything.')


def main():
    parser = ArgumentParser()
    parser.add_argument(
        '--gp-master-data',
        metavar='PATH',
        help='path to Greenplum master',
        required=True,
    )
    parser.add_argument(
        '--gp-bin',
        metavar='PATH',
        help='path to Greenplum utils',
        required=True,
    )
    parser.add_argument(
        '--master-fqdns',
        help='Greenplum master fqdns separated by comma',
        required=True,
    )
    parser.add_argument(
        '--local-fqdn',
        help='Local Greenplum host fqdn',
        required=True,
    )
    init_logging()
    args = parser.parse_args()
    master_fqdns = args.master_fqdns.split(',')

    if len(master_fqdns) != 2:
        log.error("Incorrect master fqdns: %s", master_fqdns)
        exit(1)

    gp=Greenplum(
        gp_bin=args.gp_bin,
        gp_master_data=args.gp_master_data,
        gp_master_fqdns=master_fqdns,
        gp_local_fqdn=args.local_fqdn
    )

    if path.exists(AUTORECOVERY_DISABLED_FLAG):
        log.info("Master autorecovery is disabled (flag found)")
        sys.exit(0)

    log.info('Start')
    try:
        gp.run()
    except Exception:
        log.exception('Got unexpected exception')
        raise


if __name__ == '__main__':
    main()
