#!/usr/bin/env python

import argparse
import time

import psycopg2
import socket
import subprocess
import logging
import sys
from enum import Enum
from os import environ, path
from json import loads

log = logging.getLogger(__name__)


class HostRole(Enum):
    master = 'm'
    replica = 'r'
    segment = 's'


def get_cur(host):
    try:
        conn = psycopg2.connect(host=host, user='gpadmin', dbname='postgres')
        return conn.cursor()
    except psycopg2.OperationalError as e:
        return None


def get_host_role(fqdn, cursor):
    cursor.execute('SELECT content, role FROM pg_catalog.gp_segment_configuration WHERE hostname = %s', (fqdn,))
    row = cursor.fetchone()
    if not row:
        return None
    content, role = row
    if content == -1:
        return HostRole.master if role == 'p' else HostRole.replica
    return HostRole.segment


def check_replica_alive(cursor):
    cursor.execute('SELECT state, sync_state FROM pg_stat_replication')
    row = cursor.fetchone()
    if not row:
        return False
    state, sync_state = row
    return state == 'streaming' and sync_state == 'sync'


def get_replica_fqdn(cursor):
    cursor.execute("SELECT hostname FROM pg_catalog.gp_segment_configuration WHERE content = -1 AND role = 'm'")
    row = cursor.fetchone()
    return row[0] if row else None


def wait_segments_up(cursor, timeout=300):
    count = 1
    start = time.time()

    while count > 0:
        cursor.execute("SELECT COUNT(*) FROM pg_catalog.gp_segment_configuration WHERE status = 'd'")
        count = cursor.fetchone()[0]
        if time.time() - start > timeout:
            log.info('Wait segments up timeout - stop waiting.')
            return False
        time.sleep(1)

    return True


def check_mirrors_alive(fqdn, cursor):
    cursor.execute(
        """
        SELECT s2.hostname, s2.status, s2.mode
        FROM pg_catalog.gp_segment_configuration s1
        LEFT JOIN pg_catalog.gp_segment_configuration s2
        ON s1.content = s2.content AND s1.hostname != s2.hostname
        WHERE s1.hostname = %s AND s1.role = 'p'
        """,
        (fqdn,)
    )
    for hostname, status, mode in cursor.fetchall():
        # segment up and synced
        if status != 'u' or mode != 's':
            log.info('Troubles with segment {}: status {}, mode {}'.format(hostname, status, mode))
            return False
    return True


def get_segments_hosts(cursor):
    cursor.execute("SELECT DISTINCT hostname FROM pg_catalog.gp_segment_configuration WHERE content > -1")
    return [x[0] for x in cursor.fetchall()]


def disable_greenplum_service():
    run_logged(['sudo', 'systemctl', 'disable', 'greenplum.service'])
    log.info('Service greenplum disabled.')


def stop_master():
    run_logged(['gpstop', '-a', '-m', '-M', 'fast'])
    log.info('Stop master.')


def switch_master(old_master_fqdn, new_master_fqdn):
    run_logged(
        [
            'gpssh', '-h', new_master_fqdn, '-e',
            'PGPORT=5432 gpactivatestandby -af -d %s' % environ['MASTER_DATA_DIRECTORY']
        ]
    )
    log.info('Greenplum activate standby.')

    run_logged(['gpssh', '-h', new_master_fqdn, '-e', 'gpstop -ma -Mfast'])
    log.info('Stop greenplum master.')

    run_logged(['gpssh', '-h', new_master_fqdn, '-e', 'sudo systemctl start greenplum.service'])
    run_logged(['gpssh', '-h', new_master_fqdn, '-e', 'sudo systemctl enable greenplum.service'])

    log.info('Start and enable greenplum new master.')

    activate_replica_from_master(master_fqdn=new_master_fqdn, replica_fqdn=old_master_fqdn)
    log.info('Activate replica.')

    log.info('Running hs on new master.')
    pillar = "{{'gpdb_master':true, 'master_fqdn':'{}'}}".format(new_master_fqdn)
    run_logged([
        'gpssh', '-h', new_master_fqdn, '-e',
        'sudo salt-call state.highstate queue=True pillar="{}"'.format(pillar)
    ], 'Done hs on new master')

    return True


def highstate_hosts(hosts):
    hs = {}
    for segment in hosts:
        log.info('Run hs for %s', segment)
        hs[segment] = subprocess.Popen(['gpssh', '-h', segment, '-e', 'sudo salt-call state.highstate queue=True'],
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    for h, proc in hs.items():
        output, err = proc.communicate()
        log.info('Done [{}]: code {}, out {}, err {}'.format(h, proc.returncode, output, err))
        if proc.returncode:
            raise subprocess.CalledProcessError(proc.returncode, h)

    return True


def activate_replica_from_master(master_fqdn, replica_fqdn):
    master_data_dir = environ['MASTER_DATA_DIRECTORY']
    backup_data_dir = path.join(path.dirname(master_data_dir), 'backup-{}'.format(path.basename(master_data_dir)))

    if path.exists(master_data_dir):
        if path.exists(backup_data_dir):
            run_logged(['rm', '-rf', backup_data_dir])
            log.info('Clean old backup dir.')
        run_logged(['mv', master_data_dir, backup_data_dir])
        log.info('Move old master data {} -> {}.'.format(master_data_dir, backup_data_dir))

    run_logged([
        'gpssh', '-h', master_fqdn, '-e',
        'gpinitstandby -a -s {}'.format(replica_fqdn)
    ])
    log.info('Init standby.')


def restart_replica(master_fqdn):
    run_logged(['gpssh', '-h', master_fqdn, '-e', 'gpinitstandby -an'])
    log.info('Init standby.')


def down_segments_on_host(master_fqdn, segment_fqdn):
    run_logged([
        'gpssh', '-h', master_fqdn, '-e',
        'gpstop -a -Mfast --host {}'.format(segment_fqdn)
    ])
    log.info('Stop segments on {}'.format(segment_fqdn))


def up_segments(master_fqdn, rebalance=True, cursor=None):
    run_logged(['gpssh', '-h', master_fqdn, '-e', 'gprecoverseg -a'])
    log.info('Recover segments.')

    if rebalance:
        if not wait_segments_up(cursor):
            log.info('Cannot wait to up all segments. Anyway try to rebalance segments.')
        run_logged(['gpssh', '-h', master_fqdn, '-e', 'gprecoverseg -ar'])
        log.info('Rebalance segments.')


def run_logged(cmd, comment='Command complete'):
    cmd_process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, err = cmd_process.communicate()
    log.info(
        "{}: cmd: '{}': code: {}, out: '{}', err: '{}'".format(
            comment, cmd, cmd_process.returncode, output, err)
    )
    if cmd_process.returncode:
        raise subprocess.CalledProcessError(cmd_process.returncode, cmd)


def init_logger():
    root = logging.getLogger()
    root.setLevel(logging.DEBUG)
    root.addHandler(logging.StreamHandler(sys.stdout))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--down', action='store_true', help='Use for pre_restart.')
    parser.add_argument('--up', action='store_true', help='Use for post_restart.')
    parser.add_argument('-r', '--rebalance', type=bool, default=True,
                        help='If True - rebalance segments on segment host recover. Use with --up option.')
    args = parser.parse_args()

    init_logger()

    fqdn = socket.getfqdn()

    cur = None
    master = None
    with open('/etc/dbaas.conf', 'r') as f:
        dbaas_conf = loads(f.read())
        cluster_hosts = dbaas_conf['cluster_hosts_info']
        for host in cluster_hosts:
            if 'greenplum_cluster.master_subcluster' in cluster_hosts[host]['roles']:
                cur = get_cur(host)
                master = host
            if cur is not None:
                break

    if cur is None:
        log.error("Cannot find alive master.")
        exit(1)

    role = get_host_role(fqdn, cur)
    if role is HostRole.master:
        try:
            if args.down:
                if not check_replica_alive(cur):
                    log.error("Cannot switch master - replica is dead or doesn't exists.")
                    exit(1)

                replica_fqdn = get_replica_fqdn(cur)
                segments = get_segments_hosts(cur)
                disable_greenplum_service()
                stop_master()
                switch_master(master, replica_fqdn)
                log.info('Running hs on segments and new replica.')
                segments.append(master)
                highstate_hosts(segments)

            if args.up:
                run_logged(['sudo', 'systemctl', 'start', 'greenplum.service'])
                log.info('Start greenplum service.')

        except subprocess.CalledProcessError as err:
            log.error(str(err))
            exit(1)

    elif role is HostRole.replica:
        if args.down:
            log.info('This is replica. Nothing to do there.')
            exit(0)

        if args.up:
            restart_replica(master)

    elif role is HostRole.segment:
        if args.down:
            if not check_mirrors_alive(fqdn, cur):
                log.error('Cannot shutdown segment: found broken mirrors.')
                exit(1)
            down_segments_on_host(master, fqdn)

        if args.up:
            up_segments(master, rebalance=args.rebalance, cursor=cur)
    else:
        log.error('Cannot recognize host role.')
        exit(1)


if __name__ == '__main__':
    main()
