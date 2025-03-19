#!/usr/bin/env python

from mysql_util import die, is_offline, connect, is_replica, get_master_fqdn, is_readonly


def filter_gtid_set(gtid_set, uuid):
    '''
    Exclude GTID sets from 'uuid' server
    '''
    if not gtid_set:
        return ''
    gtid_list = gtid_set.split(',')
    return str.join(',', [s.strip() for s in gtid_list if not s.startswith(uuid)])


def get_excess_gtid_set(master_fqdn, replica_gtid):
    request = "SELECT GTID_SUBTRACT('{replica_gtid}', @@GLOBAL.GTID_EXECUTED), @@SERVER_UUID".format(replica_gtid=replica_gtid)
    try:
        with connect(host = master_fqdn) as conn:
            cur = conn.cursor()
            cur.execute(request)
            res = cur.fetchall()
            if res is None:
                die(1, 'cannot check gtid set on master ({master_fqdn})'.format(master_fqdn = master_fqdn))
            return filter_gtid_set(res[0][0], res[0][1])
    except Exception as e:
        if is_offline():
            die(1, 'Mysql on {host} is offline'.format(host = master_fqdn))
        die(1, 'exception: %s' % repr(e))


def check():
    try:
        with connect() as conn:
            cur = conn.cursor()
            if not is_replica(cur):
                die(0, "OK, it's master")
            if not is_readonly(cur):
                die(2, "replica is not read-only, possible splitbrain!")
            cur.execute("SELECT @@GLOBAL.GTID_EXECUTED")
            res = cur.fetchall()
            if res is None:
                die(1, 'cannot get current gtid set')
            replica_gtid = res[0][0]
    except Exception as e:
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'exception: %s' % repr(e))
    master_fqdn = get_master_fqdn()
    result = get_excess_gtid_set(master_fqdn, replica_gtid)
    if not result:
        die(0, 'OK')
    die(2, 'splitbrain detected! Diff from master: {0}'.format(result.replace("\n", ", ")))


def _main():
    try:
        check()
    except Exception as exc:
        die(1, 'exception: %s' % repr(exc))


if __name__ == '__main__':
    _main()
