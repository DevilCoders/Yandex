#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
from contextlib import closing
import datetime
import logging
import logging.handlers
import os
import os.path
import subprocess
import shutil
import time
from typing import List
import yaml

import MySQLdb
import MySQLdb.cursors as cursors


HIGH_PORT = 3308
MAX_ALLOWED_PACKET = 512 * 1024**2


def get_logger():
    """
    Initialize logger
    """
    logging.basicConfig()
    log = logging.getLogger()
    log.setLevel(logging.DEBUG)
    return log


def get_oldbinlog_dir(walg_config):
    with open(walg_config) as fh:
        return yaml.safe_load(fh.read())['WALG_MYSQL_BINLOG_DST']


def create_oldbinlog_dir(ob_dir):
    if not os.path.exists(ob_dir):
        os.makedirs(ob_dir)


def remove_oldbinlog_dir(ob_dir):
    if os.path.exists(ob_dir):
        shutil.rmtree(ob_dir)


def remove_old_redo_log(log):
    log.info('Removing recovery redo log:')
    cmd = 'rm /var/lib/mysql/xtrabackup_logfile'
    log.info("executing cmd: %s", cmd)
    if subprocess.run(cmd, shell=True, check=False).returncode != 0:
        log.info("Failed to remove unneeded redo log")


def run_mysql_on_high_port(log, defaults_file, high_port=HIGH_PORT, max_allowed_packet=MAX_ALLOWED_PACKET):
    """
    runs mysql on high port
    """
    if subprocess.call('pgrep mysqld', shell=True) == 0:
        log.error('MySQL proc is already running')
        raise Exception('MySQL proc is already running')

    possible_options = str(subprocess.check_output(['/usr/sbin/mysqld', '--help', '--verbose']))
    if 'admin-port' in possible_options:
        admin_port_option = 'admin-port'
    else:
        admin_port_option = 'extra-port'

    log.info('running MySQL on high port')
    cmd = (
        '/usr/sbin/mysqld '
        '--socket=/tmp/mysqld.sock '
        '--pid-file=/tmp/mysql.pid '
        '--event-scheduler=OFF '
        '--max-allowed-packet={max_allowed_packet} '
        '--wait-timeout=28800 '
        '--slave-rows-search-algorithms=INDEX_SCAN,HASH_SCAN '
        '--innodb-flush-log-at-trx-commit=0 '
        '--innodb-doublewrite=OFF '
        '--sync-binlog=100 '
        '--port={high_port} '
        '--{admin_port_option}=0 '
        '--offline-mode=OFF '
        '--skip-slave-start '
        '--super-read-only=0 '
        '--disabled_storage_engines="" '
    ).format(high_port=high_port, admin_port_option=admin_port_option, max_allowed_packet=int(max_allowed_packet))
    log.info("executing cmd: %s", cmd)

    return subprocess.Popen(cmd, shell=True)


def wait_mysql_on_high_port_started(log, defaults_file, high_port=HIGH_PORT):
    cmd = 'my-wait-started -w 1000s --defaults-file={mycnf} --port={port}'.format(mycnf=defaults_file, port=high_port)
    log.info("executing cmd: %s", cmd)
    if subprocess.call(cmd, shell=True) == 0:
        return

    raise Exception('failed to run mysql on high port')


def set_gtid_purged_from_snapshot(log, datadir, connection, force_gtid_purged=False):
    binlog_info_path = os.path.join(datadir, 'xtrabackup_binlog_info')

    if not os.path.exists(binlog_info_path):
        log.info('{path} does not exists, there is no need in updating GTID_EXECUTED.'.format(path=binlog_info_path))
        return

    with open(binlog_info_path, 'r') as fhndl:
        # GTID set may be split across multiple lines
        line = "".join(l.strip() for l in fhndl.readlines())
        snapshot_gtid_executed = line.split('\t')[2].strip()

    cur = connection.cursor()

    sql = (
        "SELECT @@GLOBAL.GTID_EXECUTED AS gtid, "
        "GTID_SUBSET(@@GLOBAL.GTID_EXECUTED, '{snapshot_gitd}') as is_subset,"
        "GTID_SUBSET('{snapshot_gitd}', @@GLOBAL.GTID_EXECUTED) as is_superset "
    ).format(snapshot_gitd=snapshot_gtid_executed)
    cur.execute(sql)
    res = cur.fetchone()
    gtid_executed, is_subset, is_superset = res['gtid'], int(res['is_subset']), int(res['is_superset'])

    log.info(
        'server GTID_EXECUTED %s xtrabackup_binlog_info GTID_EXECUTED: %s is_subset: %s, is_superset: %s',
        gtid_executed,
        snapshot_gtid_executed,
        is_subset,
        is_superset,
    )

    if is_superset and not force_gtid_purged:
        log.info('server has more recent GTIDS, no update needed')
        return

    if not is_subset and not force_gtid_purged:
        raise Exception(
            'MySQL has incorrect GTID_EXECUTED. ' 'It does NOT include GTID_EXECUTED from xtrabackup_binlog_info'
        )

    sqls = [
        "RESET MASTER",
        "SET @@GLOBAL.GTID_PURGED='{snapshot_gitd}'".format(snapshot_gitd=snapshot_gtid_executed),
    ]

    for sql in sqls:
        log.info("Executing sql: {sql}".format(sql=sql))
        cur.execute(sql)


def reset_old_slave_status(conn):
    '''
    If backup was made on the slave node, we need to clear mysql.slave_master_info
    '''
    cur = conn.cursor()
    cur.execute("STOP SLAVE")
    cur.execute("RESET SLAVE ALL")


def run_upgrade(log, defaults_file, high_port=HIGH_PORT):
    log.info("running upgrade")
    cmd = """
        mysql_upgrade --defaults-file={mycnf} --port={port} mysql
    """.format(
        mycnf=defaults_file, port=high_port
    ).strip()
    log.info("executing cmd: %s", cmd)
    subprocess.call(cmd, shell=True)


def set_master_writable(conn):
    cur = conn.cursor()
    cur.execute("SET GLOBAL super_read_only = 0")


def get_gtids(cur):
    cur.execute("SHOW MASTER STATUS")
    res = cur.fetchone()
    return res.get('Executed_Gtid_Set', '').strip().replace('\n', '')


def replay_binlogs(log, backup_name, stop_datetime, until_binlog_datetime, walg_config):
    if not stop_datetime:
        stop_datetime = (datetime.datetime.now() + datetime.timedelta(days=365)).strftime("%Y-%m-%dT%H:%M:%SZ")
    log.info('Starting replaying binlogs since %s until %s', backup_name, stop_datetime)
    walg_cmd = [
        '/usr/bin/wal-g-mysql',
        'binlog-replay',
        '--since=' + backup_name,
        '--until=' + stop_datetime,
        '--config=' + walg_config,
    ]
    if until_binlog_datetime:
        walg_cmd.append('--until-binlog-last-modified-time=' + until_binlog_datetime)
        log.info('Don\'t replay binlogs that was created after %s', until_binlog_datetime)

    log.debug('Executing cmd %s', walg_cmd)

    walg_ps = subprocess.Popen(walg_cmd, cwd='/tmp')
    walg_ps.wait()
    if walg_ps.returncode != 0:
        raise Exception('wal-g exited with {code}'.format(code=walg_ps.returncode))


def _is_datadir_empty(datadir, ignore_autocnf=False):
    for entry in os.listdir(datadir):
        if entry == 'auto.cnf' and ignore_autocnf:
            continue
        if entry in ['lost+found', '.tmp']:
            continue
        return False
    return True


def ssh_fetch_backup(log, datadir: str, server: str, ssh_user: str, backup_threads):
    cmd = "rm -rf {datadir}/lost+found {datadir}/.tmp".format(datadir=datadir)
    log.debug("Executing {cmd}".format(cmd=cmd))
    if subprocess.run(cmd, shell=True, check=False).returncode != 0:
        log.info("Failed to clean datadir")

    cmd = (
        "ssh {ssh_user}@{server} 'xtrabackup --backup --stream=xbstream "
        " --lock-ddl --lock-ddl-timeout=3600 --parallel={backup_threads}' "
        " | xbstream --extract --directory={datadir} --parallel={backup_threads}"
    ).format(ssh_user=ssh_user, server=server, datadir=datadir, backup_threads=max(1, int(backup_threads)))
    log.debug("Executing {cmd}".format(cmd=cmd))
    if subprocess.call(cmd, shell=True) != 0:
        raise Exception('Failed to fetch backup.')


def xtrabackup_prepare(log, datadir: str, use_memory):
    cmd = "xtrabackup --prepare --target-dir={datadir} ".format(datadir=datadir)
    if use_memory is not None:
        cmd += " --use-memory={use_memory} ".format(use_memory=use_memory)
    log.debug("Executing {cmd}".format(cmd=cmd))
    if subprocess.call(cmd, shell=True) != 0:
        raise Exception('Failed to run `xtrabackup --prepare`.')


def find_master(log, my_hostname: str, hosts: List[str], defaults_file: str, port=3306, timeout=300):
    """
    Find master within hosts
    """
    if my_hostname in hosts:
        hosts.remove(my_hostname)

    log.info("Trying to find master in %s", hosts)
    deadline = time.time() + timeout
    while time.time() < deadline:
        for host in hosts:
            conn = None
            try:
                conn = MySQLdb.connect(
                    host=host,
                    read_default_file=defaults_file,
                    port=port,  # user=user, passwd=password,
                    db='mysql',
                    ssl={'ca': '/etc/mysql/ssl/allCAs.pem'},
                )
                cur = conn.cursor()
                cur.execute('SHOW SLAVE STATUS')
                res = cur.fetchall()
                if len(res) == 0:
                    return host
            except Exception as e:
                log.warn('failed to check slave status on host %s: %s', host, e)
            finally:
                if conn:
                    conn.close()
        time.sleep(1)
    log.error('Failed to find mysql master within %s', timeout)
    return None


def shutdown_mysql_on_high_port(proc):
    """
    Shutdowns mysql
    """
    if proc is None:
        return

    # send SIGTERM to MySQL to graceful shutdown
    proc.terminate()
    proc.wait()

    if proc.returncode != 0:
        raise Exception('failed to shutdown mysql on high port')


def main():
    """
    Console entry-point
    """
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    def add_common_args(parser):
        # Common options
        parser.add_argument('-d', '--datadir', type=str, default='/var/lib/mysql', help='mysql datadir path')
        parser.add_argument(
            '--defaults-file',
            type=str,
            default=os.path.expanduser("~/.my.cnf"),
            help='path for my.cnf with login/password for cluster',
        )
        parser.add_argument(
            '--port',
            type=int,
            default=HIGH_PORT,
            help='port for connect to local mysql',
        )
        parser.add_argument(
            '--max-allowed-packet',
            type=int,
            default=MAX_ALLOWED_PACKET,
            help='max_allowed_packet settings to replay binlogs',
        )

    # walg-fetch
    walg_parser = subparsers.add_parser('walg-fetch')
    walg_parser.set_defaults(func=walg_fetch_handler)
    add_common_args(walg_parser)
    walg_parser.add_argument(
        '--walg-config',
        type=str,
        default='/etc/wal-g/wal-g.yaml',
        help='path for wal-g configuration file',
    )
    walg_parser.add_argument('--backup-name', type=str, default='LATEST', help='Base backup name')
    walg_parser.add_argument(
        '--use_memory',
        '--use-memory',
        type=str,
        help='how much memory use for preparing backup',
    )
    walg_parser.add_argument(
        '--run-upgrade',
        type=bool,
        default=False,
        help='run mysql_upgrade on fetched snapshot',
    )

    # ssh-fetch
    ssh_parser = subparsers.add_parser('ssh-fetch')
    ssh_parser.set_defaults(func=ssh_fetch_handler)
    add_common_args(ssh_parser)
    ssh_parser.add_argument(
        '--server',
        type=str,
        default='',
        help='specify hosts (usually master) to fetch data from',
    )
    ssh_parser.add_argument(
        '--ssh-user',
        type=str,
        default='mysql',
        help='username used to connect to server',
    )
    ssh_parser.add_argument(
        '--hosts',
        type=str,
        default='',
        help='comma separated list of hosts in cluster. When --server not specified, script tries to find master from this list',
    )
    ssh_parser.add_argument(
        '--localhost',
        type=str,
        default='',
        help='localhost FQDN (it excluded from --hosts)',
    )
    ssh_parser.add_argument(
        '--backup_threads',
        '--backup-threads',
        type=int,
        default=2,
        help='how many files to fetch concurrently',
    )
    ssh_parser.add_argument(
        '--use_memory',
        '--use-memory',
        type=str,
        default='1G',
        help='how much memory use for preparing backup',
    )

    binlog_parser = subparsers.add_parser('apply-binlogs')
    binlog_parser.set_defaults(func=apply_binlogs_handler)
    add_common_args(binlog_parser)
    binlog_parser.add_argument(
        '--walg-config',
        type=str,
        default='/etc/wal-g/wal-g.yaml',
        help='path for wal-g configuration file',
    )
    binlog_parser.add_argument('--backup-name', type=str, default='LATEST', help='Base backup name')
    binlog_parser.add_argument(
        '--stop-datetime',
        type=str,
        default='',
        help='Time for point-in-time recovery (ISO format)',
    )
    binlog_parser.add_argument(
        '--until-binlog-last-modified-time',
        type=str,
        default='',
        help='Time in RFC3339 that is used to prevent wal-g from replaying binlogs '
        'that was created/modified after this time',
    )
    binlog_parser.add_argument(
        '--skip-pitr',
        type=bool,
        default=False,
        help='Skip point-in-time recovery, just set GTID',
    )

    log = get_logger()
    args = parser.parse_args()
    args.func(log, args)


def walg_fetch_handler(log, args):
    if not _is_datadir_empty(args.datadir, ignore_autocnf=True):
        raise Exception('{datadir} not empty.'.format(datadir=args.datadir))

    proc = None
    try:
        cmd = "rm -rf {datadir}/* {datadir}/.tmp/*".format(datadir=args.datadir)
        if subprocess.run(cmd, shell=True, check=False).returncode != 0:
            log.info("Failed to clean datadir")

        cmd = ("wal-g-mysql backup-fetch {backup_name} --config {walg_config} --turbo").format(
            backup_name=args.backup_name, walg_config=args.walg_config
        )
        if subprocess.call(cmd, shell=True) != 0:
            raise Exception('Failed to fetch backup.')

        xtrabackup_prepare(log, args.datadir, args.use_memory)

        proc = run_mysql_on_high_port(log, args.defaults_file, args.port, args.max_allowed_packet)
        wait_mysql_on_high_port_started(log, args.defaults_file, args.port)
        with closing(
            MySQLdb.connect(
                db='mysql', read_default_file=args.defaults_file, port=args.port, cursorclass=cursors.DictCursor
            )
        ) as conn:
            set_gtid_purged_from_snapshot(log, args.datadir, conn, True)
            reset_old_slave_status(conn)
            if args.run_upgrade:
                run_upgrade(log, args.defaults_file, args.port)
    except Exception as e:
        log.exception('walg_fetch failed due to %s', e)
        raise
    finally:
        shutdown_mysql_on_high_port(proc)
    remove_old_redo_log(log)


def ssh_fetch_handler(log, args):
    if not _is_datadir_empty(args.datadir, ignore_autocnf=True):
        raise Exception('{datadir} not empty.'.format(datadir=args.datadir))

    proc = None
    try:
        server = args.server
        if server == '':
            hosts = args.hosts.split(',')
            server = find_master(log, args.localhost, hosts, args.defaults_file)
            if server is None:
                raise Exception('no `server` specified, no master found')

        ssh_fetch_backup(log, args.datadir, server, args.ssh_user, args.backup_threads)
        xtrabackup_prepare(log, args.datadir, args.use_memory)

        proc = run_mysql_on_high_port(log, args.defaults_file, args.port, args.max_allowed_packet)
        wait_mysql_on_high_port_started(log, args.defaults_file, args.port)
        with closing(
            MySQLdb.connect(
                db='mysql', read_default_file=args.defaults_file, port=args.port, cursorclass=cursors.DictCursor
            )
        ) as conn:
            set_gtid_purged_from_snapshot(log, args.datadir, conn, True)
            reset_old_slave_status(conn)
    except Exception as e:
        log.exception('ssh_fetch failed due to %s', e)
        raise
    finally:
        shutdown_mysql_on_high_port(proc)
    remove_old_redo_log(log)


def apply_binlogs_handler(log, args):
    proc = None
    ob_dir = get_oldbinlog_dir(args.walg_config)

    try:
        proc = run_mysql_on_high_port(log, args.defaults_file, args.port, args.max_allowed_packet)
        wait_mysql_on_high_port_started(log, args.defaults_file, args.port)
        with closing(
            MySQLdb.connect(
                db='mysql', read_default_file=args.defaults_file, port=args.port, cursorclass=cursors.DictCursor
            )
        ) as conn:
            set_master_writable(conn)
            if not args.skip_pitr:
                create_oldbinlog_dir(ob_dir)
                replay_binlogs(
                    log, args.backup_name, args.stop_datetime, args.until_binlog_last_modified_time, args.walg_config
                )
    except Exception as e:
        log.exception('apply_binlog failed due to %s', e)
        raise
    finally:
        remove_oldbinlog_dir(ob_dir)
        shutdown_mysql_on_high_port(proc)


if __name__ == '__main__':
    main()
