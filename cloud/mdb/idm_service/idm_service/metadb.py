"""Database-related logic."""
import json
import logging
import time
from typing import NamedTuple
from functools import partial

from .dbutils import get_cursor
from .exceptions import (
    ClusterNotExistsError,
    ClusterStatusNotRunningError,
    DatabaseError,
    IdmServiceError,
    UnsupportedClusterType,
)
from .mysql_pillar import MysqlPillar
from .pg_pillar import PgPillar
from .pg_metadb import get_pg_user_modify_task_params
from .mysql_metadb import get_mysql_user_modify_task_params
from .utils import send_password_email

# pylint: disable=invalid-name
logger = logging.getLogger(__name__)

json_dump = partial(json.dumps, ensure_ascii=False)

_GET_IDM_CLUSTERS = """
SELECT
    c.cid, c.name, folder_id, code.rev(c) AS rev, c.type
FROM
    dbaas.clusters c
WHERE
    cid IN (
        SELECT cid
          FROM dbaas.pillar
         WHERE value @> '{"data": {"sox_audit": true}}'::jsonb
           AND cid IS NOT NULL
    )
    AND code.visible(c)"""

_GET_IDM_CLUSTERS_WITH_PILLARS = """
SELECT
    c.cid, c.name, folder_id, code.rev(c) AS rev, c.type, p.value
FROM
    dbaas.clusters c
    JOIN dbaas.pillar p
    USING (cid)
WHERE
    p.value @> '{"data": {"sox_audit": true}}'::jsonb
    AND c.type in ('postgresql_cluster', 'mysql_cluster')
    AND code.visible(c)"""

_LOCK_CLUSTER = """
SELECT
    c.cid, c.name, c.folder_id, rev, c.type, c.status, pillar_value
FROM
    code.lock_cluster(i_cid => %s) c
"""
_UPDATE_CLUSTER_PILLAR = """
SELECT code.update_pillar(
    i_cid   => %(cid)s,
    i_value => %(value)s,
    i_rev   => %(rev)s,
    i_key   => code.make_pillar_key(
        i_cid => %(cid)s
    )
)
"""
_COMPLETE_CHANGE = "SELECT code.complete_cluster_change(%s, %s)"
_ADD_OPERATION = """
SELECT *
  FROM code.add_operation(
    i_operation_id    => %(task_id)s,
    i_cid             => %(cid)s,
    i_rev             => %(rev)s,
    i_folder_id       => %(folder_id)s,
    i_task_type       => %(task_type)s,
    i_operation_type  => %(task_type)s,
    i_task_args       => %(task_args)s,
    i_metadata        => %(metadata)s,
    i_user_id         => %(created_by)s,
    i_version         => %(version)s
)
"""

_GET_OPERATION_STATUS = """
SELECT end_ts, result
FROM dbaas.worker_queue WHERE task_id = %(task_id)s;
"""

ClusterInfo = NamedTuple(
    'ClusterInfo',
    [
        ('cid', str),
        ('name', str),
        ('folder_id', int),
        ('rev', int),
        ('type', str),
    ],
)


class MetaDB:
    """A MetaDB data provider."""

    def __init__(self):
        self.crypto = None
        self.connstrings = None
        self.eager_modify = None
        self.pass_lifespan = None
        self.smtp_host = None

    def init_metadb(self, config):
        """If eager_modify is True, add cluster modify task
        after each pillar modification."""
        self.crypto = config['CRYPTO']
        self.connstrings = config['CONNSTRINGS']
        self.eager_modify = config['EAGER_MODIFY']
        self.pass_lifespan = config['PASS_LIFESPAN_DAYS']
        self.smtp_host = config['SMTP_HOST']

    def _get_clusters(self):
        with get_cursor(self.connstrings, need_master=False, err_msg='Failed to get clusters/users info') as cur:
            cur.execute(_GET_IDM_CLUSTERS)
            return [ClusterInfo(*r) for r in cur.fetchall()]

    def _get_clusters_with_pillars(self):
        with get_cursor(self.connstrings, need_master=False, err_msg='Failed to get clusters/users info') as cur:
            cur.execute(_GET_IDM_CLUSTERS_WITH_PILLARS)
            return [self._get_cluster_info_with_pillar(r) for r in cur.fetchall()]

    def _get_cluster_info_with_pillar(self, r):
        info = ClusterInfo(*r[0:5])
        if info.type == 'mysql_cluster':
            return (info, MysqlPillar(r[-1]))
        elif info.type == 'postgresql_cluster':
            return (info, PgPillar(r[-1]))
        else:
            raise UnsupportedClusterType(info.type)

    def get_clusters(self):
        """Return list of clusters managed by IDM.

        Result format:
            [ClusterInfo(cid, name, folder_id, rev), ClusterInfo(...), ...]
        """
        return self._get_clusters()

    def get_clusters_roles(self):
        """Return users/roles within each IDM managed cluster.

        Result format:
            {
                ('cid1', 'cname1'): {
                    'user1': ['reader', 'writer'],
                    'user2': ['reader'],
                },
                ('cid2', 'cname2'): {
                    'user3': ['writer'],
                },
                ...
            }
        """
        data = self._get_clusters_with_pillars()
        result = {}

        for cluster, pillar in data:
            idm_users = pillar.get_idm_users()
            if idm_users:
                result[(cluster.cid, cluster.name)] = idm_users
        return result

    def _lock_cluster(self, cid, cur):
        cur.execute(_LOCK_CLUSTER, (cid,))
        data = cur.fetchone()
        if not data:
            raise ClusterNotExistsError('Failed to find requested cluster "{}"'.format(cid))
        status = data[5]
        if status != 'RUNNING':
            raise ClusterStatusNotRunningError('Cluster {cid} has status {status}'.format(cid=cid, status=status))
        return self._get_cluster_info_with_pillar(data)

    def _complete_change(self, cid, rev, cur):
        cur.execute(_COMPLETE_CHANGE, (cid, rev))

    def _wait_task_complete(self, task_id, cur, timeout=900):
        # Wait task and return it's status
        start = time.time()

        logger.info('Wait task %s', task_id)

        while time.time() - start < timeout:
            cur.execute(_GET_OPERATION_STATUS, {'task_id': task_id})
            data = cur.fetchone()
            if not data:
                logger.warning('Not found operation %s status', task_id)
                return False

            end_ts, result = data
            if end_ts:
                logger.info('Operation %s finished. Result %s, End timestamp %s', task_id, result, end_ts)
                return result

        logger.warning('Operation %s timeout', task_id)
        return None

    def _set_pillar(self, cid, pillar, rev, cur):
        params = {
            "cid": cid,
            "value": json_dump(pillar),
            "rev": rev,
        }
        cur.execute(_UPDATE_CLUSTER_PILLAR, params)
        logger.info('Updated pillar for %s', cid)

    def _add_modify_task(self, cluster, pillar, folder_id, rev, cur, login=None):
        """Add cluster modify task."""
        if cluster.type == 'mysql_cluster':
            task_id, params = get_mysql_user_modify_task_params(cluster.cid, pillar, folder_id, rev, login)
        elif cluster.type == 'postgresql_cluster':
            task_id, params = get_pg_user_modify_task_params(cluster.cid, folder_id, rev, login)
        else:
            raise UnsupportedClusterType(cluster.type)

        cur.execute(_ADD_OPERATION, params)
        logger.info('Added cluster modify task for %s', cluster.cid)
        return task_id

    def _modify_grants(self, cid, operation, login, role):
        with get_cursor(self.connstrings, need_master=True, err_msg='Failed to {}'.format(operation)) as cur:
            cluster, pillar = self._lock_cluster(cid, cur)
            if getattr(pillar, operation)(login, role):
                self._set_pillar(cid=cluster.cid, pillar=pillar.data, rev=cluster.rev, cur=cur)
                if self.eager_modify:
                    self._add_modify_task(
                        cluster=cluster,
                        pillar=pillar,
                        folder_id=cluster.folder_id,
                        rev=cluster.rev,
                        cur=cur,
                        login=login,
                    )
                self._complete_change(cid=cluster.cid, rev=cluster.rev, cur=cur)
                logger.info('%s(%s) at %s', operation, (login, role), cluster.name)

    def grant_role(self, login, cid, role):
        """Grant role to the user. Create new user if needed."""
        self._modify_grants(cid, 'add_role', login, role)

    def revoke_role(self, login, cid, role):
        """Revoke role from the user."""
        self._modify_grants(cid, 'remove_role', login, role)

    def _modify_resps(self, cid, operation, login):
        with get_cursor(self.connstrings, need_master=True, err_msg='Failed to {}'.format(operation)) as cur:
            cluster, pillar = self._lock_cluster(cid, cur)
            if getattr(pillar, operation)(login):
                self._set_pillar(cid=cluster.cid, pillar=pillar.data, rev=cluster.rev, cur=cur)
                self._complete_change(cid=cluster.cid, rev=cluster.rev, cur=cur)
                logger.info('%s(%s) at %s', operation, login, cluster.name)

    def add_resp(self, login, cid):
        """Add resp to cluster"""
        self._modify_resps(cid, 'add_resp', login)

    def remove_resp(self, login, cid):
        """Remove resp to cluster"""
        self._modify_resps(cid, 'remove_resp', login)

    def _rotate_passwords(self, cid, cur):
        """Rotate passwords at the cluster."""
        cluster, pillar = self._lock_cluster(cid, cur)

        new_passwords = pillar.rotate_passwords(self.crypto, self.pass_lifespan)
        if new_passwords:
            self._set_pillar(cid=cluster.cid, pillar=pillar.data, rev=cluster.rev, cur=cur)

            task_id = None
            if self.eager_modify:
                task_id = self._add_modify_task(
                    cluster=cluster, pillar=pillar, folder_id=cluster.folder_id, rev=cluster.rev, cur=cur
                )
            self._complete_change(cid=cluster.cid, rev=cluster.rev, cur=cur)

            # TODO: dirty hack for fix idm spamming when cluster locked or etc
            if task_id:
                # It needs for dbaas_worker can see new operation
                cur.connection.commit()
                task_ok = self._wait_task_complete(task_id, cur)
                if not task_ok:
                    return {}

        return new_passwords or {}

    def rotate_passwords(self):
        """Rotate all stale passwords."""
        data = self._get_clusters_with_pillars()
        for cluster, pillar in data:
            try:
                if not pillar.have_stale_passwords(self.pass_lifespan):
                    continue
                err_msg = 'Failed to rotate passwords at {}'.format(cluster.cid)
                with get_cursor(self.connstrings, need_master=True, err_msg=err_msg) as cur:
                    new_passwords = self._rotate_passwords(cid=cluster.cid, cur=cur)
                    logger.info('Rotated passwords at %s', cluster.name)
                for login, password in new_passwords.items():
                    send_password_email(login, password, cluster, host=self.smtp_host)
            except IdmServiceError as error:
                logger.error(f'Failed to rotate passwords at {cluster.cid} ({cluster.name}): {error}')
                continue

    def ping(self):
        """Check the database connection."""
        with get_cursor(self.connstrings, need_master=True, err_msg='Ping failed') as cur:
            cur.execute("SELECT 1")
            (result,) = cur.fetchone()
            if result != 1:
                raise DatabaseError('Unexpected behavior')


DB = MetaDB()
