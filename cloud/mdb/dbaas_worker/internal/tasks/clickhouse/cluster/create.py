"""
Clickhouse Create cluster executor
"""
from typing import Any, Optional

from ..utils import cache_user_object, ch_build_host_groups, classify_host_map
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import select_host, group_host_list_by_shard_names, register_executor
from ....providers.clickhouse.backup_key import BackupKey
from ....providers.clickhouse.cloud_storage import CloudStorage
from ....providers.billingdb.billingdb import BillingDBProvider, BillType
from ....types import HostGroup
from ....utils import get_first_key, to_host_list

CREATE = 'clickhouse_cluster_create'

HostPillar = dict[str, Any]


@register_executor(CREATE)
class ClickHouseClusterCreate(ClusterCreateExecutor):
    """
    Create Clickhouse cluster in dbm and/or compute
    """

    _selected_ch_host: str = ''

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.ch_backup_key = BackupKey(self.config, self.task, self.queue)
        self.cloud_storage = CloudStorage(self.config, self.task, self.queue, self.args)
        self.billingdb = BillingDBProvider(self.config, self.task, self.queue, self.args)

    def _get_selected_ch_host_from_ch_hosts(self, ch_hosts: Optional[dict] = None) -> str:
        if not ch_hosts:
            if self._selected_ch_host:
                return self._selected_ch_host
            else:
                raise ValueError('could not get selected ch host as ch_hosts has not been passed.')

        self._selected_ch_host = get_first_key(ch_hosts)  # type: ignore
        return self._selected_ch_host

    def _before_highstate(self):
        ch_hosts, _ = classify_host_map(self.args['hosts'])
        ch_subcid = next(iter(ch_hosts.values()))['subcid']

        self.ch_backup_key.exists(ch_subcid)

        if self._is_compute(ch_hosts) and self.cloud_storage.has_cloud_storage_bucket():
            self.cloud_storage.protect(ch_subcid)
            self.cloud_storage.set_billing_tag()
        elif self._is_aws(ch_hosts):
            self.cloud_storage.aws_protect(ch_subcid)
            self.billingdb.enable_billing(cid=self.task['cid'], bill_type=BillType.CH_CLOUD_STORAGE)

        selected_ch_host = self._get_selected_ch_host_from_ch_hosts(ch_hosts)

        if self.args.get('update-geobase', False):
            cache_user_object(
                self.deploy_api,
                selected_ch_host,
                ch_hosts[selected_ch_host]['environment'],
                'geobase',
                'custom_clickhouse_geobase',
            )

    def run(self):
        self.acquire_lock()

        ch_group, zk_group = ch_build_host_groups(self.args['hosts'], self.config)

        groups = [self._prepare_ch_group(ch_group)]
        if zk_group.hosts:
            groups = [zk_group] + groups

        self._create(*groups)

        # handle backup creation
        initial_backup_info = self.args.get('backup_service_initial_info', [])
        if len(initial_backup_info) > 0:
            for backup in initial_backup_info:
                self.backup_db.schedule_initial_backup(
                    cid=self.task['cid'],
                    subcid=backup['subcid'],
                    shard_id=backup['shard_id'],
                    backup_id=backup['backup_id'],
                )

        self._health_host_group(ch_group)
        self._health_host_group(zk_group)

        self.release_lock()

    def _prepare_ch_group(self, ch_group: HostGroup) -> HostGroup:
        make_pillar_kwargs = self._get_make_pillar_kwargs()

        for shard_name, hosts in group_host_list_by_shard_names(to_host_list(ch_group.hosts)).items():
            make_pillar_kwargs['shard_name'] = shard_name

            common_pillar, selected_pillar = self._make_pillars(make_pillar_kwargs)

            for fqdn in [host['fqdn'] for host in hosts]:
                ch_group.hosts[fqdn]['service_account_id'] = self.args.get('service_account_id')  # type: ignore
                ch_group.hosts[fqdn]['deploy'] = {  # type: ignore
                    'pillar': common_pillar,
                    'title': 'common',
                }

            # We need to run specific highstate on one host in cluster shard
            # E.g. on restore data insert should be done only on one host
            selected_host = select_host(hosts)
            ch_group.hosts[selected_host]['deploy'] = {  # type: ignore
                'pillar': selected_pillar,
                'title': 'selected',
            }

        return ch_group

    def _get_make_pillar_kwargs(self) -> dict[str, Any]:
        """
        Returns keyword arguments for pillar creation.
        """
        return {}

    def _make_pillars(self, kwargs: dict[str, Any]) -> tuple[HostPillar, HostPillar]:
        """
        Make pillars for common and selected hosts.
        """
        return self._make_pillar(), self._make_pillar(selected=True)

    def _make_pillar(self, selected: bool = False, **kwargs) -> HostPillar:
        """
        Make pillar for host creation:
        1. data restore or backup purposes if `selected`
        2. schema restore or nothing special, otherwise
        """
        ret = {'restart-check': False}
        if selected and len(self.args.get('backup_service_initial_info', [])) == 0:
            ret['do-backup'] = True

        return ret
