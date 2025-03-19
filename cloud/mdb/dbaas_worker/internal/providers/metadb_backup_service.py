"""
MetaDB backups manipulation module
"""

import time

from dbaas_common import retry, tracing

from ..metadb import DatabaseConnectionError
from ..query import execute
from .base_metadb import BaseMetaDBProvider
from .common import Change
from ..exceptions import ExposedException


class BackupCreateError(ExposedException):
    """
    Backup Creation error
    """


class MetadbBackups(BaseMetaDBProvider):
    """
    DBaaS MetaDB BackupService provider
    """

    def wait_completed(self, backup_id):
        """
        Wait for backup status to became completed (or error)
        """
        self.wait_for_status(backup_id, ['DONE'], ['CREATE-ERROR'])

    def wait_deleted(self, backup_id):
        """
        Wait for backup status to became deleted (or error)
        """
        self.wait_for_status(backup_id, ['DELETED'], ['DELETE-ERROR'])

    def wait_for_status(self, backup_id, ok_statuses, error_statuses):
        """
        Wait for backup status to became needed status (or error)
        """
        context = {
            'backup_id': backup_id,
        }

        with self.interruptable:
            while True:
                status = self.get_backup_info(backup_id)['status']
                if status in ok_statuses:
                    self.add_change(
                        Change(
                            'backup-status-{backup_id}'.format(**context),
                            'ok',
                            context=context,
                            rollback=Change.noop_rollback,
                        )
                    )
                    return

                elif status in error_statuses:
                    self.add_change(
                        Change(
                            'backup-status-{backup_id}'.format(**context),
                            'fail',
                            context=context,
                            rollback=Change.noop_rollback,
                        )
                    )
                    raise BackupCreateError(f'Backup with backup_id: {backup_id} creation failed')

                else:
                    time.sleep(10)

    # pylint: disable=no-member
    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Select')
    def get_backup_info(self, backup_id):
        """
        Get backup information
        """
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                ret = execute(
                    cur,
                    'get_backup_info',
                    fetch=True,
                    **{
                        'backup_id': backup_id,
                    },
                )
                if len(ret) > 0:
                    return ret[0]
                return None

    # pylint: disable=no-member
    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Update')
    def delete_cluster_backups(self, cid):
        """
        Mark cluster backups deleted and drop data
        """
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                execute(cur, 'mark_cluster_backups_deleted', fetch=False, cid=cid)

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Update')
    def schedule_initial_backup(self, cid, subcid, shard_id, backup_id, start_delay=0):
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                execute(
                    cur,
                    'schedule_initial_backup',
                    fetch=False,
                    cid=cid,
                    subcid=subcid,
                    shard_id=shard_id,
                    backup_id=backup_id,
                    delay_seconds=start_delay,
                )
