#!/usr/bin/env python

import argparse
import sys
import time
import psycopg2
from json import loads
from socket import getfqdn
from os.path import exists

control_data = '/var/lib/greenplum/data1/master/gpseg-1/global/pg_control'


def get_cur(host, args):
    try:
        conn = psycopg2.connect(host=host, user='gpadmin', dbname='postgres')
        return conn.cursor()
    except Exception as err:
        if args.verbose:
            print(time.strftime('%H:%M:%S ') + str(err).rstrip())
        return None


def get_master_hosts():
    with open('/etc/dbaas.conf', 'r') as f:
        dbaas_conf = loads(f.read())
        cluster_hosts = dbaas_conf['cluster_hosts_info']
        master_hosts = []
        for host in cluster_hosts:
            if 'greenplum_cluster.master_subcluster' in cluster_hosts[host]['roles']:
                master_hosts.append(host)
    return master_hosts


def gp_is_ok(args, master_hosts, fqdn):
    for host in master_hosts:
        cur = get_cur(host, args)
        if cur:
            cur.execute('SELECT count(*) FROM pg_catalog.gp_segment_configuration')
            count = cur.fetchone()[0]
            if count >= 3:
                if fqdn != host and fqdn in master_hosts:
                    if args.verbose:
                        print(time.strftime('%H:%M:%S We are on standby, skip'))
                    return True
                return True
            else:
                if args.verbose:
                    print(time.strftime('%H:%M:%S Got %s', count))
    return False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', type=int, default=5432, help='Greenplum port')
    parser.add_argument('-w', '--wait', type=int, default=600, help='Time to wait (0 for infinite)')
    parser.add_argument('-v', '--verbose', action="store_true", help='Some logging')
    args = parser.parse_args()

    start = time.time()
    master_hosts = get_master_hosts()
    fqdn = getfqdn()

    while args.wait == 0 or time.time() < start + args.wait:
        if gp_is_ok(args, master_hosts, fqdn):
            print(time.strftime('%H:%M:%S Done'))
            sys.exit(0)
        else:
            time.sleep(5)
    if args.verbose:
        print('Timeout (%d seconds) expired.' % args.wait)

    sys.exit(1)


if __name__ == '__main__':
    main()
