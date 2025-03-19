#!/usr/bin/env python3
"""
Retry deletion of delete-error backups
"""

import logging
from contextlib import closing
from secrets import choice

import psycopg2

BACKUPS_QUERY = """
SELECT
    b.cid, b.backup_id
FROM
    dbaas.backups b
    JOIN dbaas.clusters c ON (b.cid = c.cid)
WHERE
    c.status = 'RUNNING'
    AND c.cid not in (
        SELECT cid
        FROM dbaas.backups
        WHERE status IN ('DELETING'::dbaas.backup_status, 'OBSOLETE'::dbaas.backup_status)
        GROUP BY cid)
    AND b.status = 'DELETE-ERROR'::dbaas.backup_status
    AND b.metadata IS NOT NULL
"""

MARK_BACKUP = """
UPDATE
    dbaas.backups
SET
    status = 'OBSOLETE'::dbaas.backup_status,
    delayed_until = now() + trunc(random()  * 360) * '1 MINUTE'::interval,
    updated_at = now(),
    shipment_id = null
WHERE
    backup_id = %(backup_id)s
"""


def retry_backups_delete():
    """
    Connect to local postgresql and retry backups delete
    """
    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
    log = logging.getLogger('deleter')
    count = 0
    backups = {}
    with closing(psycopg2.connect('dbname=dbaas_metadb host=localhost')) as conn:
        with conn as txn:
            cursor = txn.cursor()
            cursor.execute('SELECT pg_is_in_recovery()')
            if cursor.fetchone()[0]:
                log.info('We are on replca')
                return
            cursor.execute(BACKUPS_QUERY)
            for row in cursor.fetchall():
                count += 1
                if row[0] not in backups:
                    backups[row[0]] = []
                backups[row[0]].append(row[1])
            log.info('Got %s DELETE-ERROR backups for %s clusters', count, len(backups))
            for cluster_backups in backups.values():
                selected = choice(cluster_backups)
                log.info('Marking backup %s as OBSOLETE', selected)
                cursor.execute(MARK_BACKUP, {'backup_id': selected})


if __name__ == '__main__':
    retry_backups_delete()
