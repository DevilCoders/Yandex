"""
Clickhouse Create cluster executor
"""
from typing import TypedDict

from ...utils import register_executor
from .create import ClickHouseClusterCreate, HostPillar


RESTORE = 'clickhouse_cluster_restore'

# Backup model for processing in worker
WorkerBackup = TypedDict(
    'WorkerBackup',
    {
        'cid': str,
        'backup-id': str,
        's3-path': str,
    },
)

# Backup model that comes from Internal API
IntApiBackup = TypedDict(
    'IntApiBackup',
    {
        'cid': str,
        'backup-id': str,
        's3-path': str,
        'shard-name': str,
    },
)


class MakePillarKwargs(TypedDict):
    """
    Arguments for common/selected pillar creations
    """

    not_backup_restore: bool
    target_pillar_id: str
    restore_from_per_shard_name: dict[str, WorkerBackup]
    shard_name: str


@register_executor(RESTORE)
class ClickHouseClusterRestore(ClickHouseClusterCreate):
    """
    Restore Clickhouse cluster in dbm and/or compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)

    def _before_highstate(self):
        super(ClickHouseClusterRestore, self)._before_highstate()

        job_id = self.deploy_api.run(
            self._selected_ch_host,
            deploy_version=self.deploy_api.get_deploy_version_from_minion(self._selected_ch_host),
            deploy_title='s3-cache-restore',
            method={
                'commands': [
                    {
                        'type': 'saltutil.sync_all',
                        'arguments': [],
                        'timeout': 600,
                    },
                    {
                        'type': 'mdb_clickhouse.restore_user_object_cache',
                        'arguments': [
                            f'backup_bucket={self.args.get("source_s3_bucket")}',
                            f'restore_geobase={self.args.get("restore-geobase", False)}',
                        ],
                        'timeout': 1800,
                    },
                ],
                'fqdns': [self._selected_ch_host],
                'parallel': 1,
                'stopOnErrorCount': 1,
                'timeout': 2400,
            },
        )
        self.deploy_api.wait([job_id])

    def _get_make_pillar_kwargs(self) -> MakePillarKwargs:  # type: ignore
        kwargs = MakePillarKwargs(
            not_backup_restore=False,
            target_pillar_id=self.args['target-pillar-id'],
            restore_from_per_shard_name={},
            shard_name='',
        )
        if 'backups' not in self.args['restore-from'].keys():
            # Because we have come here not from backup restore
            kwargs['not_backup_restore'] = True
        else:
            kwargs['restore_from_per_shard_name'] = self._map_restore_from_to_shard_names()

        return kwargs

    def _make_pillars(self, kwargs: MakePillarKwargs) -> tuple[HostPillar, HostPillar]:  # type: ignore
        if kwargs['not_backup_restore']:
            restore_from = self.args['restore-from']
        else:
            restore_from = kwargs['restore_from_per_shard_name'][kwargs['shard_name']]

        common_pillar = self._make_pillar(
            target_pillar_id=kwargs['target_pillar_id'],
            restore_from=restore_from,
        )
        selected_pillar = self._make_pillar(
            selected=True,
            target_pillar_id=kwargs['target_pillar_id'],
            restore_from=restore_from,
        )

        return common_pillar, selected_pillar

    def _make_pillar(self, selected: bool = False, **kwargs) -> HostPillar:
        ret: HostPillar = super(ClickHouseClusterRestore, self)._make_pillar(selected=selected, **kwargs)

        ret.update(
            {
                'target-pillar-id': kwargs['target_pillar_id'],
                'restore-from': dict(**kwargs['restore_from']),
            }
        )
        if not selected:
            ret['restore-from']['schema-only'] = True

        if 'do-backup' in ret:
            ret['do-backup'] = kwargs.get('not_backup_restore', False)

        return ret

    def _map_restore_from_to_shard_names(self) -> dict[str, WorkerBackup]:
        """
        Unpacks backup info to separate `restore-from`s and map them to according shard names
        """
        cid: str = self.args['restore-from']['cid']
        backups: list[IntApiBackup] = self.args['restore-from'].get('backups', [])

        res: dict[str, WorkerBackup] = {}
        for backup in backups:
            res[backup['shard-name']] = {
                "cid": cid,
                "backup-id": backup['backup-id'],
                "s3-path": backup['s3-path'],
            }

        return res
