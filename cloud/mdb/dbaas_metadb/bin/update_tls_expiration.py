import psycopg2
import json
import argparse

from contextlib import closing
from OpenSSL.crypto import load_certificate, FILETYPE_PEM
from nacl.encoding import URLSafeBase64Encoder
from nacl.public import Box, PrivateKey, PublicKey

cluster_type_to_some_operation = {
    'postgresql_cluster': 'postgresql_cluster_modify',
    'clickhouse_cluster': 'clickhouse_cluster_modify',
    'mongodb_cluster': 'mongodb_cluster_modify',
    'redis_cluster': 'redis_cluster_modify',
    'mysql_cluster': 'mysql_cluster_modify',
    'sqlserver_cluster': 'sqlserver_cluster_modify',
    'greenplum_cluster': 'greenplum_cluster_start',  # there is no operation like greenplum_cluster_modify or smth
    'hadoop_cluster': 'hadoop_cluster_modify',
    'kafka_cluster': 'kafka_cluster_modify',
    'elasticsearch_cluster': 'elasticsearch_cluster_modify',
}


def decrypt(box, encrypted_msg):
    decoded = URLSafeBase64Encoder.decode(encrypted_msg["data"].encode("utf-8"))
    return box.decrypt(decoded).decode("utf-8")


def get_expiration(cert):
    x509 = load_certificate(FILETYPE_PEM, cert)
    # YYYYMMDDhhmmssZ
    expiration_openssl_format = x509.get_notAfter()
    # YYYY-MM-DDThh:mm:ssZ
    expiration_swagger_format = '{Y}-{M}-{D}T{h}:{m}:{s}Z'.format(
        Y=expiration_openssl_format[0:4].decode('utf-8'),
        M=expiration_openssl_format[4:6].decode('utf-8'),
        D=expiration_openssl_format[6:8].decode('utf-8'),
        h=expiration_openssl_format[8:10].decode('utf-8'),
        m=expiration_openssl_format[10:12].decode('utf-8'),
        s=expiration_openssl_format[12:14].decode('utf-8'),
    )
    return expiration_swagger_format


def get_cids():
    with closing(psycopg2.connect(database="dbaas_metadb", user="postgres", host="localhost", port=5432)) as conn:
        cur = conn.cursor()
        cur.execute(
            "SELECT distinct cluster.cid "
            "FROM dbaas.clusters cluster "
            "JOIN dbaas.subclusters USING (cid) "
            "JOIN dbaas.hosts host USING (subcid) "
            "JOIN dbaas.pillar on pillar.fqdn = host.fqdn "
            "WHERE dbaas.visible_cluster_status(status) AND pillar.value->'cert.crt' IS NOT NULL"
        )
        res = cur.fetchall()
        print('selected {N} cids'.format(N=len(res)))
        return res


def run_finished_before_starting_task(cid, cur, conn):
    cur.execute('select type ,status, folder_id, actual_rev from dbaas.clusters where cid=%(cid)s;', {'cid': cid})
    type, status, folder_id, rev = cur.fetchone()
    if status not in ('RUNNING', 'STOPPED'):
        return
    operation_type = cluster_type_to_some_operation[type]
    cur.execute(
        """select code.add_finished_operation_for_current_rev(
                                i_operation_id => gen_random_uuid()::text,
                                i_cid => %(cid)s,
                                i_folder_id => %(folder_id)s,
                                i_operation_type => %(operation_type)s,
                                i_metadata => '{}'::jsonb,
                                i_user_id => 'migration',
                                i_version => 2,
                                i_hidden => true,
                                i_rev => %(rev)s)""",
        {'cid': cid, 'rev': rev, 'folder_id': folder_id, 'operation_type': operation_type},
    )
    conn.commit()


def change_pillars(cids, box):
    with closing(psycopg2.connect(database="dbaas_metadb", user="postgres", host="localhost", port=5432)) as conn:
        cur = conn.cursor()
        n = len(cids)
        i = 0
        for cid in cids:
            cur.execute('select status, folder_id from dbaas.clusters where cid=%(cid)s;', {'cid': cid})
            status, folder_id = cur.fetchone()
            if status not in ('RUNNING', 'STOPPED'):
                print('Can not update {cid} in status {status}'.format(cid=cid, status=status))
                continue
            cur.execute(
                'select fqdn from dbaas.hosts join dbaas.subclusters using (subcid) where cid=%(cid)s', {'cid': cid}
            )
            fqdns = cur.fetchall()
            print(fqdns)
            cur.execute('select rev from code.lock_cluster(%(cid)s)', {'cid': cid})
            rev = cur.fetchone()[0]
            fqdns = [e[0] for e in fqdns]
            for fqdn in fqdns:
                cur.execute(
                    'select value from dbaas.pillar where fqdn=%(fqdn)s and value->\'cert.crt\' is not null;',
                    {'fqdn': fqdn},
                )
                res = cur.fetchone()
                if res is None:
                    continue
                pillar_value = res[0]
                cert = pillar_value['cert.crt']
                decrypted_cert = decrypt(box, cert)
                pillar_value['cert.expiration'] = get_expiration(decrypted_cert)

                cur.execute(
                    """select code.update_pillar(
                        i_cid := %(cid)s,
                        i_rev := %(rev)s,
                        i_key := code.make_pillar_key(i_fqdn := %(fqdn)s),
                        i_value := %(pillar_value)s::jsonb
                    )""",
                    {'cid': cid, 'rev': rev, 'pillar_value': json.dumps(pillar_value), 'fqdn': fqdn},
                )
            cur.execute('select code.complete_cluster_change(%(cid)s, %(rev)s);', {'cid': cid, 'rev': rev})
            conn.commit()
            run_finished_before_starting_task(cid, cur, conn)
            i += 1
            print("{i}/{n} cids updated".format(i=i, n=n))


def main():
    """
    Console entry-point
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--internal-api-public-key")
    parser.add_argument("--salt-private-key")

    args = parser.parse_args()

    box = Box(
        PrivateKey(args.salt_private_key.encode("utf-8"), URLSafeBase64Encoder),
        PublicKey(args.internal_api_public_key.encode("utf-8"), URLSafeBase64Encoder),
    )

    cids = get_cids()
    change_pillars(cids, box)


if __name__ == '__main__':
    main()
