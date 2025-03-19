import argparse
import datetime
import json
import psycopg2
import pytz
from contextlib import closing


def log(s, debug=False):
    if debug:
        print(s)


def get_hosts_expiration_info(host, password, debug):
    with closing(psycopg2.connect(database="secretsdb", user="secrets_api",
                                  host=host, port=6432,
                                  password=password)) as conn:
        cur = conn.cursor()
        cur.execute("select host, expiration from secrets.certs")
        res = cur.fetchall()
        log('selected {N} hosts with expiration time'.format(N=len(res)), debug)
        return res


def update_expiration_in_pillars(info, debug):
    cids = set()
    utc_tz = pytz.utc
    with closing(psycopg2.connect(database="dbaas_metadb", user="postgres",
                                  host="localhost", port=5432)) as conn:
        conn.autocommit = False
        cur = conn.cursor()
        n = len(info)
        i = 0
        for host, ts in info:
            cur.execute("""select clusters.cid from dbaas.subclusters, dbaas.hosts, dbaas.clusters
where subclusters.subcid=hosts.subcid
and hosts.fqdn=%(host)s
and subclusters.cid=clusters.cid
and clusters.status in ('RUNNING', 'STOPPED')
limit 1""",
                        {'host': host})
            res = cur.fetchone()
            if res is None:
                log("can't find cid of {host}".format(host=host), debug)
                i += 1
                log('{i} / {n} hosts are processed'.format(i=i, n=n), debug)
                continue
            cid = res[0]
            conn.commit()
            cur.execute('select value from dbaas.pillar where fqdn=%(host)s and value->\'cert.expiration\' is null;',
                        {'host': host})
            res = cur.fetchone()
            if res is None:
                log("host {host} pillar has already been updated".format(host=host), debug)
                i += 1
                log('{i} / {n} hosts are processed'.format(i=i, n=n), debug)
                continue
            pillar_value = res[0]
            uts_ts = ts.astimezone(utc_tz)
            pillar_value['cert.expiration'] = str(uts_ts.strftime("%Y-%m-%dT%H:%M:%SZ"))
            cur.execute('select rev from code.lock_cluster(%(cid)s)', {'cid': cid})
            rev = cur.fetchone()[0]
            log('Ready for update {cid}'.format(cid=cid), debug)
            cur.execute("""select code.update_pillar(
        i_cid := %(cid)s,
        i_rev := %(rev)s,
        i_key := code.make_pillar_key(i_fqdn := %(host)s),
        i_value := %(pillar_value)s::jsonb
    )""",
                        {'cid': cid,
                         'rev': rev,
                         'pillar_value': json.dumps(pillar_value),
                         'host': host})

            cur.execute('select code.complete_cluster_change(%(cid)s, %(rev)s);', {'cid': cid, 'rev': rev})
            conn.commit()
            cids.add(cid)
            i += 1
            log('{i} / {n} hosts are processed'.format(i=i, n=n), debug)


def main():
    """
    Console entry-point
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--secrets-password',
        type=str,
        help="Password for user secrets_api"
    )
    parser.add_argument(
        '--secrets-host',
        type=str,
        help="secretsdb host"
    )
    parser.add_argument(
        '--debug',
        action='store_true'
    )

    args = parser.parse_args()
    debug = args.debug
    if not args.secrets_host or not args.secrets_password:
        log('secrets_host, secrets_password should be defined.', True)
        return
    info = get_hosts_expiration_info(args.secrets_host, args.secrets_password, debug)
    update_expiration_in_pillars(info, debug)


if __name__ == '__main__':
    main()
