#!/usr/bin/env python3

import json
import os
import socket
import time

from dns.resolver import query

BLM_VERSION = (8, 0, 25)
DBAAS_CONF_PATH = '/etc/dbaas.conf'
MYSYNC_CACHE_PATH = '/var/run/mysync/mysync.info'
MYSQL_MONITOR_USER = '~monitor/.my.cnf'
CID_MASTER_TEMPLATE = 'c-%s.rw.%s'
CID_REPLICA_TEMPLATE = 'c-%s.ro.%s'
STATUS_TIMEFRESH_SEC = 30
ALLOW_TYPES = ['postgresql_cluster', 'mysql_cluster', 'redis_cluster', 'elasticsearch_cluster']
STATE_PATHS = {
    'postgresql_cluster': '/tmp/.pgsync_db_state.cache',
    'redis_cluster': '/tmp/.redis_state.cache',
    'elasticsearch_cluster': '/tmp/.elasticsearch_state.cache'
}


class OldStateError(RuntimeError):
    pass


def safe_close_connection(connection):
    try:
        if connection:
            connection.close()
    except Exception:
        pass


def get_cluster_hosts(err_base_msg):
    try:
        conf = get_dbaas_conf()
        return conf['cluster_hosts']
    except Exception as exc:
        return (1, err_base_msg + ", exception while reading '%s': %s" % (DBAAS_CONF_PATH, repr(exc)))


def check_postgres_replica_health(err_base_msg, rname, port=3306):
    import psycopg2
    hosts = get_cluster_hosts(err_base_msg)

    if rname not in hosts:
        return (1, err_base_msg + ", resolved replica '%s' is unknown to master" % rname)

    replica_conn = None
    try:
        replica_conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=%s' % rname)
        with replica_conn.cursor() as repl_cur:
            repl_cur.execute('select pg_is_in_recovery()')
            res = repl_cur.fetchall()

            if not res[0]:
                return (1, err_base_msg + ", replica '%s' is not in recovery" % rname)

            return (0, "OK, replica: '%s'" % rname)

    except Exception:
        return (2, err_base_msg + ", resolved replica '%s' is probably dead" % (rname))
    finally:
        safe_close_connection(replica_conn)


def is_mysql_master_alive(rname):
    from mysql_util import connect, is_replica
    try:
        with connect(host=rname) as conn:
            return not is_replica(conn.cursor())
    except Exception as e:
        return False

def is_mysql_replica_alive(rname):
    from mysql_util import connect, is_replica
    try:
        with connect(host=rname, connect_timeout=1) as conn:
            return is_replica(conn.cursor())
    except Exception as e:
        return False


def check_mysql_replica_health(err_base_msg, rname, port=3306):
    hosts = get_cluster_hosts(err_base_msg)

    if len(hosts) <= 1:
        return (0, 'OK, there are no replicas')

    if rname not in hosts:
        return (1, err_base_msg + ", resolved replica '%s' is unknown to master" % rname)

    if is_mysql_replica_alive(rname):
        return (0, "OK, replica: '%s'" % rname)

    return (2, err_base_msg + ", resolved replica '%s' is probably dead or not a replica" % rname)


def check_replica_health_if_needed(err_base_msg, ct, rname):
    if ct == 'mysql_cluster':
        return check_mysql_replica_health(err_base_msg, rname)

    if ct == 'postgresql_cluster':
        return check_postgres_replica_health(err_base_msg, rname)

    return (0, 'OK')


def get_dbaas_conf():
    """
    Get cluster configuraiton file
    """
    path = DBAAS_CONF_PATH
    return json.load(open(path))


def get_db_state(path):
    """
    Get last fresh pg state
    """
    mtime = os.stat(path).st_mtime
    diff_time = time.time() - mtime
    if diff_time > STATUS_TIMEFRESH_SEC:
        raise OldStateError('too old state file "%s", updated %d sec ago' %
                            (path, diff_time))
    with open(path) as fobj:
        state = json.load(fobj)
        return state


def is_master(ct, fqdn):
    if ct == "mysql_cluster":
        return is_mysql_master_alive(fqdn)
    dbstate = get_db_state(STATE_PATHS[ct])
    return dbstate.get("role") == "master"


def get_cname(qname, optional=False):
    try:
        dns_response = query(qname, 'cname')  # list of returned names
    except Exception as e:
        if not optional:
            raise e
        dns_response = []

    if len(dns_response) > 1:
        raise RuntimeError("unexpectedly got multiple records for cname: %d" % len(dns_response))
    if not dns_response:
        return ""
    return dns_response[0].to_text()[:-1]


def check():
    dbc = get_dbaas_conf()
    ct = dbc.get("cluster_type")
    if ct not in ALLOW_TYPES:
        return (1, "cluster type is %s, supported only following: '%s'" % (
            ct, ", ".join(ALLOW_TYPES)))

    cid = dbc.get("cluster_id")
    fqdn = socket.getfqdn()

    if not is_master(ct, fqdn):
        return (0, "OK, type: %s, not a master host" % ct)

    parts = fqdn.split('.')
    for i in range(len(parts) - 1, 0, -1):
        if parts[i] in ('db', 'mdb'):
            break
    suffix = '.'.join(parts[i:])
    cidname = CID_MASTER_TEMPLATE % (cid, suffix)

    err_base_msg = "type: '%s', cid: '%s', fqdn: '%s', alias: '%s'" % (
        ct, cid, fqdn, cidname)

    try:
        mname = get_cname(cidname)
    except Exception as exc:
        return (2, err_base_msg + ", no cname for master host, err: " + repr(exc))

    if ct == "redis_cluster":
        if mname == fqdn:
            return (0, "OK")
        try:
            with open("/etc/redis/cluster.conf") as cluster:
                try:
                    ip = socket.gethostbyname(mname)
                except Exception:
                    ip = socket.getaddrinfo(mname, None, socket.AF_INET6)[0][4][0]
                for line in cluster:
                    if line.find(ip) == -1:
                        continue
                    if line.find("master") == -1:
                        return (2, err_base_msg + ", wrong master alias: '%s'" % (mname))
                    return (0, "OK")
        except Exception:
            return (2, err_base_msg + ", wrong master alias: '%s'" % (mname))

    else:
        cid_replica = CID_REPLICA_TEMPLATE % (cid, suffix)
        rname = get_cname(cid_replica, optional=True)

        if mname != fqdn:
            return (2, err_base_msg + ", wrong master alias: '%s', replica: '%s'" % (
                mname, rname))

        replica_err_base_msg = "type: '%s', cid: '%s', fqdn: '%s', alias: '%s'" % (
            ct, cid, fqdn, cid_replica)

        if rname == fqdn:
            # alias for postgres with one added or removed replica will reference master
            if ct == 'postgresql_cluster' and len(dbc['cluster_hosts']) <= 2:
                return (0, "OK, postgres replica resolved to master itself")
            return (1, replica_err_base_msg + ", wrong replica resolved to master host")

        return check_replica_health_if_needed(replica_err_base_msg, ct, rname)


def _main():
    try:
        code, msg = check()
        print('%d;%s' % (code, msg))
    except Exception as exc:
        print('1;exception: %s' % repr(exc))


if __name__ == '__main__':
    _main()
