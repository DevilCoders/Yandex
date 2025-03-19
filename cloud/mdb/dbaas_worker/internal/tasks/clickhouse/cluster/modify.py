"""
ClickHouse Modify cluster executor
"""
import concurrent
from copy import deepcopy
from math import ceil
from types import SimpleNamespace
from typing import Mapping
from collections import defaultdict

from ....exceptions import ExposedException
from ....providers.clickhouse.cloud_storage import CloudStorage
from ....providers.s3_bucket import S3Bucket
from ....utils import get_first_key, get_first_value
from ...common.create import BaseCreateExecutor
from ...common.delete import BaseDeleteExecutor
from ...common.cluster.modify import ClusterModifyExecutor
from ...utils import (
    build_host_group,
    build_host_group_from_list,
    register_executor,
    split_hosts_for_modify_by_shard_batches,
    group_host_list_by_shards,
    to_host_list,
    HostGroup,
)
from ..utils import cache_user_object, ch_build_host_groups, create_schema_backup, delete_ch_backup


class ClusterNotHealthyError(ExposedException):
    """
    Bad cluster health error
    """


@register_executor('clickhouse_cluster_modify')
class ClickHouseClusterModify(BaseCreateExecutor, ClusterModifyExecutor, BaseDeleteExecutor):
    """
    Modify clickhouse cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        to_create = self.args.get('hosts_to_create', [])
        to_update = list(fqdn for fqdn in self.args['hosts'] if fqdn not in to_create)
        to_ignore = self.args.get('ignore_hosts', [])

        create_ch_group, create_zk_group = ch_build_host_groups(
            self.args['hosts'], self.config, allowed_hosts=to_create, ignored_hosts=to_ignore
        )
        if create_zk_group.hosts:
            raise RuntimeError("Unsupported ZooKeeper host creation now")

        created_host_group, backup_id_by_host = self._create_ch_hosts(create_ch_group)
        keeper_hosts = self.args.get('keeper_hosts', {})
        if len(keeper_hosts) > 0:
            keepers_without_data_exist = False in keeper_hosts.values()
            if keepers_without_data_exist:
                created_host_group, to_update = self._handle_new_keepers(
                    created_host_group, backup_id_by_host, to_update, to_ignore
                )

        if len(to_update) > 0:
            self._update_hosts(to_update, to_ignore)
        if len(created_host_group.hosts) > 0:
            self._run_hs_and_cleanup_new_hosts(created_host_group, backup_id_by_host)

        self._delete_ch_hosts()

        self.mlock.unlock_cluster()

    def _handle_new_keepers(self, created_host_group, backup_id_by_host, to_update, to_ignore: list = None):
        handled_hosts = []
        keeper_hosts = self.args.get('keeper_hosts', {})
        if len(keeper_hosts) == 1:
            # for the case of downscale
            single_keeper = get_first_key(keeper_hosts)
            self._update_hosts([single_keeper], to_ignore)
            handled_hosts.append(single_keeper)
        elif len(keeper_hosts) > 1:
            # We need to run HS simultaneously on at least 2 hosts, to maintain the quorum
            # try to select these hosts from different shards
            fqdns_to_run_hs_first = self._choose_keepers_to_run_hs_first(keeper_hosts, self.args['hosts'])

            # run HS concurrently on 2 keepers
            with concurrent.futures.ThreadPoolExecutor(max_workers=2) as executor:
                futures = []
                for fqdn in fqdns_to_run_hs_first:
                    if fqdn in created_host_group.hosts:
                        created_ch_keeper = HostGroup(
                            properties=deepcopy(created_host_group.properties),
                            hosts={fqdn: created_host_group.hosts[fqdn]},
                        )
                        futures.append(
                            executor.submit(self._run_hs_and_cleanup_new_hosts, created_ch_keeper, backup_id_by_host)
                        )
                    else:
                        futures.append(executor.submit(self._update_hosts, [fqdn], to_ignore))
                    handled_hosts.append(fqdn)

                for future in concurrent.futures.as_completed(futures):
                    future.result()

        intact_new_hosts = HostGroup(
            properties=deepcopy(created_host_group.properties),
            hosts={key: created_host_group.hosts[key] for key in created_host_group.hosts if key not in handled_hosts},
        )
        intact_update_host = [fqdn for fqdn in to_update if fqdn not in handled_hosts]
        return intact_new_hosts, intact_update_host

    @staticmethod
    def _choose_keepers_to_run_hs_first(keeper_hosts: dict[str, bool], all_hosts: dict[str, dict]) -> list[str]:
        shard_to_keeper_with_data = defaultdict(list)  # type: Mapping[str, list[str]]
        shard_to_keepers_without_data = defaultdict(list)  # type: Mapping[str, list[str]]
        keeper_shards = set()
        for keeper_fqdn, has_data in keeper_hosts.items():
            keeper_shard = all_hosts[keeper_fqdn]['shard_id']
            keeper_shards.add(keeper_shard)
            if has_data:
                shard_to_keeper_with_data[keeper_shard].append(keeper_fqdn)
            else:
                shard_to_keepers_without_data[keeper_shard].append(keeper_fqdn)

        fqdns_to_run_hs_first: list[str] = []
        if len(keeper_shards) == 1:
            # Running inside one shard
            fqdns_to_run_hs_first.extend(get_first_value(shard_to_keeper_with_data))
            fqdns_to_run_hs_first.extend(get_first_value(shard_to_keepers_without_data))
            return fqdns_to_run_hs_first[:2]

        if len(shard_to_keeper_with_data) > 1:
            first_shard, second_shard, *_ = shard_to_keeper_with_data.keys()
            return [
                next(iter(shard_to_keeper_with_data[first_shard])),
                next(iter(shard_to_keeper_with_data[second_shard])),
            ]

        keeper_shard = get_first_key(shard_to_keeper_with_data)
        for dataless_keeper_shard in shard_to_keepers_without_data.keys():
            if dataless_keeper_shard != keeper_shard:
                return [
                    next(iter(shard_to_keepers_without_data[dataless_keeper_shard])),
                    next(iter(shard_to_keeper_with_data[keeper_shard])),
                ]

        raise RuntimeError('Could not select keeper hosts')

    def _update_hosts(self, to_update, to_ignore: list = None):
        modify_ch_group, modify_zk_group = ch_build_host_groups(
            self.args['hosts'], self.config, allowed_hosts=to_update, ignored_hosts=to_ignore
        )
        self._enable_cloud_storage(modify_ch_group)
        self._update_geobase(modify_ch_group)
        self._modify_cluster(modify_ch_group, modify_zk_group)

    def _enable_cloud_storage(self, ch_group):
        if self.args.get('enable_cloud_storage', False):
            s3_bucket = S3Bucket(self.config, self.task, self.queue)
            s3_bucket.exists(self.args.get('s3_buckets', {}).get('cloud_storage'), 'cloud_storage')

            if self._is_compute(ch_group.hosts):
                cloud_storage = CloudStorage(self.config, self.task, self.queue, self.args)
                ch_subcid = get_first_value(ch_group.hosts)['subcid']
                cloud_storage.protect(ch_subcid)
                cloud_storage.set_billing_tag()
            elif self._is_aws(ch_group.hosts):
                cloud_storage = CloudStorage(self.config, self.task, self.queue, self.args)
                ch_subcid = get_first_value(ch_group.hosts)['subcid']
                cloud_storage.aws_protect(ch_subcid)

            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        'cloud-storage-enable',
                        self.args['hosts'][host]['environment'],
                    )
                    for host in ch_group.hosts
                ]
            )

    def _update_geobase(self, ch_group):
        if self.args.get('update-geobase', False):
            selected_ch_host = get_first_key(ch_group.hosts)
            cache_user_object(
                self.deploy_api,
                selected_ch_host,
                ch_group.hosts[selected_ch_host]['environment'],
                'geobase',
                'custom_clickhouse_geobase',
                remove_on_error=False,
            )

    def _modify_cluster(self, ch_group, zk_group):
        for host in ch_group.hosts:
            ch_group.hosts[host]['service_account_id'] = self.args.get('service_account_id')
            ch_group.hosts[host]['deploy'] = {
                'pillar': {
                    'update-geobase': self.args.get('update-geobase', False),
                    'update-firewall': self.args.get('update-firewall', False),
                },
            }

        if zk_group.hosts:
            majority = ceil((len(zk_group.hosts) + 1) / 2)
            allow_fail_hosts = len(zk_group.hosts) - majority
            order = self._get_zk_order_for_modify(zk_group, allow_fail_hosts=allow_fail_hosts)
            self._modify_hosts(
                zk_group.properties,
                hosts=zk_group.hosts,
                order=order,
                health_check_each=True,
                allow_fail_hosts=allow_fail_hosts,
            )

        if not self.args.get('restart'):
            self._modify_hosts(ch_group.properties, hosts=ch_group.hosts)
        else:
            self._modify_hosts_by_shard_batches(ch_group.properties, hosts=ch_group.hosts)

    def _modify_hosts_by_shard_batches(self, properties: SimpleNamespace, hosts: dict) -> None:
        all_hosts = build_host_group(properties, hosts)
        for i_hosts in split_hosts_for_modify_by_shard_batches(hosts, fast_mode=self.args.get('fast_mode', False)):
            self._modify_hosts(properties, hosts=i_hosts)
            self._health_host_group(all_hosts)

    def _get_zk_order_for_modify(self, zk_group, allow_fail_hosts):
        context_suffix = '-before-zk-upgrade'
        self._health_host_group(zk_group, context_suffix=context_suffix, allow_fail_hosts=allow_fail_hosts)
        zk_hosts_ok, zk_hosts_unknown = self._split_hosts_by_health(zk_group, context_suffix=context_suffix)
        order = zk_hosts_unknown
        order.extend(zk_hosts_ok)
        return order

    def _create_ch_hosts(self, host_group: HostGroup) -> tuple[HostGroup, dict[str, dict]]:
        """
        Creates new ClickHouse hosts/shards
        """
        if len(host_group.hosts) == 0:
            return host_group, {}

        backup_info_by_shard_id = {}
        backup_id_by_host = {}
        if self.args.get('resetup_from_replica', False):
            for shard, hosts in group_host_list_by_shards(to_host_list(host_group.hosts)).items():
                backup_host, backup_shard_name = self._choose_backup_host(
                    self.args['hosts'], self.args.get('hosts_to_create', []), shard, hosts
                )
                if backup_host not in backup_id_by_host:
                    backup_id_by_host[backup_host] = create_schema_backup(self.deploy_api, backup_host)
                backup_info_by_shard_id[shard] = {
                    'replica_schema_backup': backup_id_by_host[backup_host],
                    'schema_backup_shard': backup_shard_name,
                    'schema_backup_host': backup_host,
                }
                if len(self.args.get('keeper_hosts', {})) > 0:
                    backup_info_by_shard_id[shard]['zk_hosts_with_data'] = [
                        keeper for keeper, has_data in self.args.get('keeper_hosts', {}).items() if has_data
                    ]

        for _, opts in host_group.hosts.items():
            opts['service_account_id'] = self.args.get('service_account_id')  # type: ignore
            opts['deploy'] = {'pillar': {'restart-check': False}}  # type: ignore
            if self.args.get('resetup_from_replica', False):
                opts['deploy']['pillar'].update(backup_info_by_shard_id[opts['shard_id']])  # type: ignore

        # TODO set do_backup flag on random host in every new shard

        self._create_cluster_service_account(host_group)
        self._create_host_secrets(host_group)
        self._set_network_parameters([host_group])
        self._create_host_group(host_group, revertable=True)
        self._wait_minions_registered(host_group)
        self._issue_tls(host_group)
        return host_group, backup_id_by_host

    def _run_hs_and_cleanup_new_hosts(self, host_group, backup_id_by_host):
        self._highstate_host_group(host_group)

        if self.args.get('resetup_from_replica', False):
            for host, backup_id in backup_id_by_host.items():
                delete_ch_backup(self.deploy_api, host, backup_id)

    @staticmethod
    def _choose_backup_host(
        all_hosts: dict[str, dict], new_hosts: list[str], shard_id: str, shard_hosts: list
    ) -> tuple[str, str]:
        """
        Finds most suitable host to copy schema from
        """
        is_new_shard = all(map(lambda h: h['fqdn'] in new_hosts, shard_hosts))
        existing_hosts = list(
            (host, opts['shard_name']) for host, opts in sorted(all_hosts.items()) if host not in new_hosts
        )
        if is_new_shard:
            return next(iter(existing_hosts))
        else:
            return next((host, shard_name) for host, shard_name in existing_hosts if shard_name == shard_id)

    def _delete_ch_hosts(self) -> None:
        to_delete = self.args.get('hosts_to_delete', [])
        if len(to_delete) == 0:
            return
        self._delete_host_group_full(build_host_group_from_list(self.config.clickhouse, to_delete))
        keeper_host = next(iter(self.args.get('keeper_hosts', [])))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    keeper_host,
                    'cleanup',
                    timeout=6 * 3600,
                    title='post-clickhouse-delete-cleanup',
                    environment=self.args['hosts'][keeper_host]['environment'],
                    pillar={
                        'clickhouse-hosts': {
                            'cid': self.task['cid'],
                            'removed-hosts': [host['fqdn'] for host in self.args.get('hosts_to_delete', [])],
                        },
                    },
                )
            ]
        )
