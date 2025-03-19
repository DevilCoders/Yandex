import psycopg2
from contextlib import closing


def update_pillars():
    with closing(psycopg2.connect(database="dbaas_metadb", user="postgres", host="localhost", port=5432)) as conn:
        cur = conn.cursor()
        cur.execute(
            """
            SELECT folder_id, cid FROM dbaas.clusters c
            JOIN dbaas.pillar p USING (cid)
            WHERE c.type = 'postgresql_cluster'
            AND (status = 'RUNNING' OR status = 'STOPPED')
            AND p.value @> '{"data": {"use_idm": true}}'::jsonb
        """
        )
        clusters = cur.fetchall()

        print('selected {N} clusters'.format(N=len(clusters)))

        for i, (folder_id, cid) in enumerate(clusters):
            cur.execute('SELECT rev FROM code.lock_cluster(%(cid)s)', {'cid': cid})
            rev = cur.fetchone()[0]

            cur.execute("UPDATE dbaas.pillar SET value = value #- '{data, use_idm}' WHERE cid = %(cid)s", {'cid': cid})

            cur.execute('SELECT code.complete_cluster_change(%(cid)s, %(rev)s);', {'cid': cid, 'rev': rev})
            cur.execute(
                """
                SELECT code.add_finished_operation_for_current_rev(
                i_operation_id => gen_random_uuid()::text,
                i_cid => %(cid)s,
                i_folder_id => %(folder_id)s,
                i_operation_type => 'drop deprecated pillar use_idm flag',
                i_metadata => '{}'::jsonb,
                i_user_id => 'migration',
                i_version => 2,
                i_hidden => true,
                i_rev => %(rev)s)
                """,
                {'cid': cid, 'rev': rev, 'folder_id': folder_id},
            )
            conn.commit()
            print("{i}/{n} cids updated".format(i=i + 1, n=len(clusters)))


def main():
    """
    Console entry-point
    """
    update_pillars()


if __name__ == '__main__':
    main()
