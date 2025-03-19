import psycopg2
from contextlib import closing


def get_cids(cluster_type, cluster_status):
    with closing(psycopg2.connect(database="dbaas_metadb", user="postgres", host="localhost", port=5432)) as conn:
        cur = conn.cursor()
        cur.execute(
            "SELECT c.cid "
            "FROM dbaas.clusters c "
            "JOIN dbaas.pillar p on p.cid = c.cid "
            "WHERE type = %(cluster_type)s "
            "AND c.status::text = ANY(%(cluster_status)s) "
            "AND (p.value->'data'->'mysql'->'config'->>'long_query_time')::numeric > 0 "
            "AND p.value->'data'->'mysql'->'config'->'slow_query_log' is null "
            # "AND env in ('prod', 'compute-prod')"
            ,
            {'cluster_type': cluster_type, 'cluster_status': cluster_status},
        )
        res = cur.fetchall()
        print('selected {N} cids'.format(N=len(res)))
        return res


def change_pillars(cids, cluster_status, value_path, new_value):
    with closing(psycopg2.connect(database="dbaas_metadb", user="postgres", host="localhost", port=5432)) as conn:
        cur = conn.cursor()
        n = len(cids)
        i = 0
        for cid in cids:
            cur.execute('select status from dbaas.clusters where cid=%(cid)s;', {'cid': cid})
            status = cur.fetchone()[0]
            if status not in cluster_status:
                print('Can not update {cid} in status {status}'.format(cid=cid, status=status))
                continue
            cur.execute(
                "select code.easy_update_pillar(%(cid)s, %(value_path)s, %(new_value)s::text::jsonb)",
                {'cid': cid, 'value_path': value_path, 'new_value': new_value},
            )

            cur.execute('select actual_rev from dbaas.clusters where cid=%(cid)s;', {'cid': cid})
            conn.commit()
            i += 1
            print("{i}/{n} cids updated".format(i=i, n=n))


def main():
    """
    Console entry-point
    """
    cluster_status = ['RUNNING', 'STOPPED']

    cids = get_cids('mysql_cluster', cluster_status)
    change_pillars(cids, cluster_status, '{data, mysql, config, slow_query_log}', True)


if __name__ == '__main__':
    main()
