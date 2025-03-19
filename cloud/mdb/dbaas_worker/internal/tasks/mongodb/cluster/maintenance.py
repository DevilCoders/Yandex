"""
MongoDB cluster maintenance executor
"""

from ....providers.common import Change
from ...common.cluster.maintenance import ClusterMaintenanceExecutor
from ...utils import build_host_group, group_host_list_by_shards, register_executor
from ..utils import (
    MONGOCFG_HOST_TYPE,
    MONGOD_HOST_TYPE,
    MONGOINFRA_HOST_TYPE,
    MONGOS_HOST_TYPE,
    ORDERED_HOST_TYPES,
    classify_host_map,
)


@register_executor('mongodb_cluster_maintenance')
class MongodbClusterMaintenance(ClusterMaintenanceExecutor):
    """
    Perform Maintenance on MongoDB cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        host_types = classify_host_map(self.args['hosts'])
        all_hgs = {}
        for host_type, hosts in host_types.items():
            all_hgs[host_type] = build_host_group(getattr(self.config, host_type), hosts)

        if self._is_offline_maintenance():
            # Sometimes too old hosts can't start without highstate(i.e. expired cert), so omit checks here.
            if 'disable_health_check' not in self.args:
                self.args['disable_health_check'] = True
            self.args['update_tls'] = True

        for group in all_hgs.values():
            self._start_offline_maintenance(group)

        update_tls = self.args.get('update_tls', False)
        run_highstate = self.args.get('run_highstate', False)
        do_restart = self.args.get('restart', False)
        do_stepdown = self.args.get('do_stepdown', False)

        pillar = {
            'service-stepdown': True,
            'service-restart': True,
        }

        # Update TLS certificates
        if update_tls:
            for host_type, hosts in host_types.items():
                host_group = build_host_group(getattr(self.config, host_type), hosts)
                self._issue_tls(host_group, force_tls_certs=True)
                self._highstate_host_group(host_group, title="update-tls")

        if do_stepdown:
            self.perform_stepdown(all_hgs[MONGOD_HOST_TYPE], do_stepdown)

        # If we want to run HS and do restart, do in in the same time, don't split to two actions
        if run_highstate and do_restart:
            if any(
                (
                    MONGOS_HOST_TYPE in host_types and host_types[MONGOS_HOST_TYPE],
                    MONGOINFRA_HOST_TYPE in host_types and host_types[MONGOINFRA_HOST_TYPE],
                )
            ):
                self.perform_sharded_highstate_with_restart()
            else:
                for host_type, hosts in host_types.items():
                    host_group = build_host_group(getattr(self.config, host_type), hosts)
                    self.perform_sequental_hs(
                        host_group,
                        pillar,
                        'maintenance-hs-restart',
                    )

        # Just run HS
        if run_highstate and not do_restart:
            for host_type, hosts in host_types.items():
                host_group = build_host_group(getattr(self.config, host_type), hosts)
                self._highstate_host_group(host_group, title="maintenance-hs")

        # Restart service if needed
        if do_restart and not run_highstate:
            for host_type, hosts in host_types.items():
                host_group = build_host_group(getattr(self.config, host_type), hosts)
                self._health_host_group(host_group, f'-pre-restart-{host_type}')
                self._run_operation_host_group(
                    host_group,
                    'service',
                    title=f'maintenance-restart-{host_type}',
                    pillar=pillar,
                    order=host_group.hosts.keys(),
                )

        for group in all_hgs.values():
            self._stop_offline_maintenance(group)
        self.mlock.unlock_cluster()

    def perform_stepdown(self, host_group, fqdn):
        '''
        Perform stepdown on MongoD instances
        '''
        masters = []
        if isinstance(fqdn, str) and fqdn in host_group.hosts.keys():
            masters.append(fqdn)
        elif fqdn is True:
            # We need to stepdown, but we do not know, who is master
            # so we need to find masters first
            state_type = 'mdb_mongodb.get_primary'

            for hg_fqdn, opts in host_group.hosts.items():
                opts['fqdn'] = hg_fqdn
            shards = group_host_list_by_shards(host_group.hosts.values())
            for shard_id, shard in shards.items():
                if not shard:
                    continue
                host = shard[0]['fqdn']
                shipment_id = self._run_sls_host(
                    host,
                    'mongod',
                    pillar={},
                    environment=self.args['hosts'][fqdn]['environment'],
                    state_type=state_type,
                    title=f'get-primary-{shard_id}',
                    rollback=Change.noop_rollback,
                )
                self.deploy_api.wait([shipment_id])
                result = self.deploy_api.get_result_for_shipment(shipment_id, host, state_type)
                if not result:
                    raise Exception(f'state not found for shipment {shipment_id}')

                if result.get('primary'):
                    masters.append(result.get('primary'))

        if masters:
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        'stepdown-host',
                        self.args['hosts'][host]['environment'],
                        title='mw-stepdown',
                        pillar={
                            'stepdown-id': self.task['task_id'],
                        },
                    )
                ]
                for host in masters
            )

    def perform_sequental_hs(self, host_group, pillar, title_suffix):
        '''
        For given host group, perform sequential HS with given pillar.
        Check health before and after HS
        '''

        self._health_host_group(host_group, f'-pre-{title_suffix}')
        for host in host_group.hosts.keys():
            self.deploy_api.wait(
                [
                    self.deploy_api.run(
                        host,
                        pillar=pillar,
                        deploy_title=title_suffix,
                    ),
                ]
            )
        self._health_host_group(host_group, f'-post-{title_suffix}')

    def perform_sharded_highstate_with_restart(self):
        '''
        Run hs with restart in sharded cluster, act like we are upgrading:
        https://docs.mongodb.com/master/release-notes/4.2-upgrade-sharded-cluster/
        '''

        hosts = classify_host_map(self.args['hosts'])
        host_groups = {
            host_type: build_host_group(getattr(self.config, host_type), hosts[host_type]) for host_type in hosts
        }

        for htype in ORDERED_HOST_TYPES:
            if htype not in host_groups:
                continue
            self._health_host_group(host_groups[htype], '-pre-balancer-stop')

        # 1. Disable the Balancer
        mongos_hlist = []
        for htype in [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE]:
            if htype not in host_groups:
                continue
            mongos_hlist += host_groups[htype].hosts

        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'balancer-stop',
                    self.args['hosts'][host]['environment'],
                    title='mongos-balancer-stop',
                )
                for host in mongos_hlist
            ]
        )

        # 2. Upgrade the config servers.
        for htype in [MONGOCFG_HOST_TYPE, MONGOINFRA_HOST_TYPE]:
            if htype not in host_groups:
                continue
            self.perform_sequental_hs(
                host_groups[htype],
                pillar={
                    'service-restart': True,
                    'service-for-restart': 'mongocfg',
                    'skip-mongos': True,
                },
                title_suffix='mongocfg-hs-restart',
            )

        # 3. Upgrade the shards.
        self.perform_sequental_hs(
            host_groups[MONGOD_HOST_TYPE],
            pillar={
                'service-stepdown': True,
                'service-restart': True,
                'service-for-restart': 'mongod',
            },
            title_suffix='mongod-hs-restart',
        )

        # 4. Upgrade the mongos instances.
        for htype in [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE]:
            if htype not in host_groups:
                continue

            self.perform_sequental_hs(
                host_groups[htype],
                pillar={
                    'service-restart': True,
                    'service-for-restart': 'mongos',
                    'skip-mongocfg': True,
                },
                title_suffix='mongos-hs-restart',
            )

        # 5. Re-enable the balancer.
        # mongos_hlist is already contain MongoS + MongoInfra
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'balancer-start',
                    self.args['hosts'][host]['environment'],
                    title='mongos-balancer-start',
                )
                for host in mongos_hlist
            ]
        )
