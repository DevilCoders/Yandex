"""
Common deploy executor
"""
import json
from typing import List, Sequence, Union

from nacl.hash import sha256

from ...providers.common import BaseProvider, Change
from ...providers.deploy import DEPLOY_VERSION_V2, DeployAPI
from ...providers.host_health import HostHealth
from ...providers.metadb_security_group import MetadbSecurityGroup
from ...providers.mlock import Mlock
from ...providers.solomon_service_alerts import SolomonServiceAlerts, SolomonServiceAlertsV2
from ...providers.vpc import VPCProvider
from ..utils import GenericHost, HostGroup, combine_sg_service_rules, get_managed_hostname, resolve_order
from .base import BaseExecutor


class BaseDeployExecutor(BaseExecutor):
    """
    Base class for create executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        # TODO: delete after migration to alerts scheme v2 is done
        self.alerts_provider = SolomonServiceAlerts(self.config, self.task, self.queue)
        self.alerts_provider_v2 = SolomonServiceAlertsV2(self.config, self.task, self.queue)
        self.deploy_api = DeployAPI(self.config, self.task, self.queue)
        self.host_health = HostHealth(self.config, self.task, self.queue)
        self.context_cache = BaseProvider(self.config, self.task, self.queue)
        self.mlock = Mlock(self.config, self.task, self.queue)
        self.local_cache = {}
        self.desired_security_groups = None
        self.metadb_sg = None
        self.vpc_sg = None
        self.use_sg = self.config.compute.use_security_group
        self.remove_allow_all_rule = False
        self.desired_service_sg_rules = None
        if self.use_sg:
            self.metadb_sg = MetadbSecurityGroup(config, task, queue)
            self.vpc_sg = VPCProvider(self.config, self.task, self.queue)

    def _delete_alerts(self, cid: str):
        """
        Sync changes of alerts in Monitoring.
        """
        self.alerts_provider.delete_by_cid(cid)

    def _sync_alerts(self, cid: str):
        """
        Sync changes of alerts in Monitoring.
        """
        self.alerts_provider.state_to_desired(cid)

    def _sync_alerts_v2(self, cid: str, monitoring_stub_request_id: str):
        """
        Sync changes of alerts in Monitoring.
        """
        self.alerts_provider_v2.state_to_desired(cid, monitoring_stub_request_id)

    @staticmethod
    def _is_compute(hosts: dict[str, dict]) -> bool:
        """
        Whether deploy is running in compute
        """
        return any(opts['vtype'] == 'compute' for opts in hosts.values())

    @staticmethod
    def _is_aws(hosts: Union[dict[str, GenericHost], dict[str, dict]]) -> bool:
        """
        Whether deploy is running in aws

        hosts: dict of hosts where key is hostname
        """
        return any(opts['vtype'] == 'aws' for opts in hosts.values())

    @staticmethod
    def _is_aws_host_groups(host_groups: list[HostGroup]) -> bool:
        return any(BaseDeployExecutor._is_aws(host_group.hosts) for host_group in host_groups)

    def _get_exec_timeout(self):
        """
        Get timeout for execute salt-calls
        """
        return self.deploy_api.get_seconds_to_deadline() // self.deploy_api.default_max_attempts

    def _wait_minions_registered(self, host_group):
        """
        Wait until minions are registered in salt master
        """
        self.deploy_api.wait_minions_registered(*host_group.hosts.keys())

    def _highstate_host_group(self, host_group, title=None, order=None):
        """
        Run highstate on whole host group
        """
        if not order:
            self.deploy_api.wait(
                [
                    self.deploy_api.run(
                        host,
                        pillar=opts.get('deploy', {}).get('pillar'),
                        deploy_title=opts.get('deploy', {}).get('title') or title,
                    )
                    for host, opts in host_group.hosts.items()
                ]
            )
        else:
            for host in resolve_order(order, 'deploy'):
                opts = host_group.hosts[host]
                self.deploy_api.wait(
                    [
                        self.deploy_api.run(
                            host,
                            pillar=opts.get('deploy', {}).get('pillar'),
                            deploy_title=opts.get('deploy', {}).get('title') or title,
                        )
                    ]
                )

    def _health_host_group(self, host_group, context_suffix='', timeout=1800, allow_fail_hosts=0):
        """
        Check health of hosts in host group
        """
        targets = [get_managed_hostname(host, opts) for host, opts in host_group.hosts.items()]
        self.host_health.wait(
            targets,
            host_group.properties.juggler_checks,
            context_suffix=context_suffix,
            timeout=timeout,
            allow_fail_hosts=allow_fail_hosts,
        )

    def _split_hosts_by_health(self, host_group, context_suffix=''):
        """
        Split hosts on dead and alive
        """
        return self.host_health.split(host_group, context_suffix=context_suffix)

    def _health_hosts(self, host_group, hosts, context_suffix='', allow_fail_hosts=0):
        """
        Check health of several hosts from host_group only
        """
        targets = [get_managed_hostname(host, host_group.hosts[host]) for host in hosts.items()]
        self.host_health.wait(
            targets,
            host_group.properties.juggler_checks,
            context_suffix=context_suffix,
            allow_fail_hosts=allow_fail_hosts,
        )

    def _run_sls_host(
        self,
        host,
        *sls,
        pillar,
        environment,
        state_type='state.sls',
        title=None,
        timeout=None,
        rollback=None,
        critical=False,
    ):
        if timeout is None:
            timeout = self._get_exec_timeout()
        deploy_version = self.deploy_api.get_deploy_version_from_minion(host)
        if pillar is None:
            pillar = {}
        pillar.update({'feature_flags': self.task['feature_flags']})
        sync_finished = self.context_cache.context_get(f'{host}.saltutil_sync')
        if deploy_version == DEPLOY_VERSION_V2:
            method = {
                'commands': [
                    {
                        'type': state_type,
                        'arguments': [
                            x,
                            'pillar={pillar}'.format(pillar=json.dumps(pillar, ensure_ascii=False)),
                            'concurrent=True',
                            'saltenv={environment}'.format(environment=environment),
                            'timeout={timeout}'.format(timeout=timeout),
                        ],
                        'timeout': timeout,
                    }
                    for x in sls
                ],
                'fqdns': [host],
                'parallel': 1,
                'stopOnErrorCount': 1,
                'timeout': timeout * len(sls),
            }
            if not sync_finished:
                method['commands'].insert(
                    0,
                    {
                        'type': 'saltutil.sync_all',
                        'arguments': [],
                        'timeout': timeout,
                    },
                )
                self.context_cache.add_change(
                    Change(
                        f'{host}.saltutil_sync',
                        True,
                        context={f'{host}.saltutil_sync': True},
                        rollback=Change.noop_rollback,
                    )
                )
                self.local_cache[f'{host}.saltutil_sync'] = True
            elif f'{host}.saltutil_sync' not in self.local_cache:
                self.context_cache.add_change(Change(f'{host}.saltutil_sync', True, rollback=Change.noop_rollback))
                self.local_cache[f'{host}.saltutil_sync'] = True
        else:
            raise Exception('invalid deploy version: {version}'.format(version=deploy_version))

        return self.deploy_api.run(
            host,
            method=method,
            pillar=pillar,
            deploy_version=deploy_version,
            deploy_title=title or ','.join(sls),
            rollback=rollback,
            critical=critical,
        )

    def _run_operation_host(self, host, operation, environment, title=None, pillar=None, timeout=None, rollback=None):
        """
        Run state.sls 'components.dbaas-operations.xxx' operation on single host
        """
        return self._run_sls_host(
            host,
            f'components.dbaas-operations.{operation}',
            environment=environment,
            pillar=pillar,
            timeout=timeout,
            title=title,
            rollback=rollback,
        )

    def _call_salt_module_host(
        self,
        host,
        operation,
        environment,
        state_type='state.sls',
        title=None,
        pillar=None,
        timeout=None,
        rollback=None,
        critical=False,
    ):
        """
        Run any operation on single host
        """
        return self._run_sls_host(
            host,
            operation,
            environment=environment,
            state_type=state_type,  # 'state.sls' or any state from _modules (e.g. 'mdb_mysql.is_replica')
            pillar=pillar,
            timeout=timeout,
            title=title,
            rollback=rollback,
            critical=critical,
        )

    def _run_operation_host_group(
        self, host_group, operation, title=None, order=None, pillar=None, timeout=None, rollback=None
    ):
        """
        Run operation on host group
        """
        if not order:
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        operation,
                        environment=opts['environment'],
                        pillar=pillar or opts.get('deploy', {}).get('pillar'),
                        timeout=timeout,
                        title=title,
                        rollback=rollback,
                    )
                    for host, opts in host_group.hosts.items()
                ]
            )
        else:
            for host in resolve_order(order, 'deploy'):
                self.deploy_api.wait(
                    [
                        self._run_operation_host(
                            host,
                            operation,
                            environment=host_group.hosts[host]['environment'],
                            pillar=pillar or host_group.hosts[host].get('deploy', {}).get('pillar'),
                            timeout=timeout,
                            title=title,
                            rollback=rollback,
                        ),
                    ]
                )

    def _sg_add_icmp_rules(self, rules):
        for protocol in ['ICMP', 'IPV6_ICMP']:
            rules.append(
                {
                    'direction': 'BOTH',
                    'protocol': protocol,
                    'ports_from': 0,
                    'ports_to': 0,
                }
            )

    def _sg_add_dns_rules(self, rules):
        """
        https://st.yandex-team.ru/MDB-16607
        https://st.yandex-team.ru/CLOUD-91804
        """
        rules.append(
            {
                'direction': 'EGRESS',
                'protocol': 'UDP',
                'ports_from': 53,
                'ports_to': 53,
                'v4_cidr_blocks': ["0.0.0.0/0"],
                'v6_cidr_blocks': ["::/0"],
                'description': "overlay DNS access",
            }
        )

    def _sg_add_allow_all_rules(self, rules):
        """
        Adds rule to mimic no-sg-assigned behavior.
        We need this so all cluster hosts always have service rules.
        """
        rules.append(
            {
                'direction': 'BOTH',
                'protocol': 'ANY',
                'description': "service_sg_allow_all_rule",
                'ports_from': 0,
                'ports_to': 65535,
                'v4_cidr_blocks': ["0.0.0.0/0"],
                'v6_cidr_blocks': ["::/0"],
            }
        )

    def sg_calc_hash(self, rules):
        """
        calculate some version hash from rules
        """
        str_rules = [
            "%s-%s-%d-%d" % (r.get('protocol', ""), r['direction'], r['ports_from'], r['ports_to']) for r in rules
        ]
        str_rules.sort()
        data = b''
        for sr in str_rules:
            data += str.encode(sr)
        return int.from_bytes(sha256(data)[:4], 'big')

    def sg_service_rules(self, host_groups: Sequence[HostGroup], allow_all: bool) -> List[dict]:
        sg_service_rules = combine_sg_service_rules(*host_groups)
        self._sg_add_icmp_rules(sg_service_rules)
        self._sg_add_dns_rules(sg_service_rules)
        if allow_all:
            self._sg_add_allow_all_rules(sg_service_rules)
        return sg_service_rules

    def sg_update_rules(self, sg_id, rules, sg_hash, sg_allow_all):
        self.vpc_sg.set_security_group_rules(sg_id, rules)
        self.metadb_sg.update_sg_hash(sg_id, sg_hash, sg_allow_all=sg_allow_all)

    def _desired_security_groups_state(self, host_groups: list, managed: bool = True):
        """
        Generic method for ensure exist and fill security groups

        1. Create SERVICE Security group if none exists.
        2. Fill self.security_groups with
          * service group
          * either
            * default group
            * groups supplied by user
        3. Remember the desired state in MetaDB (but do not store Default in meta).
        """
        if not self.use_sg:
            return

        # should have sg_info with network_id
        sg_info = self.metadb_sg.get_sgroup_info(self.task['cid'])
        # You can find default SG only by querying network.
        # You will not see it on instance.network_interface[*].security_group_id,
        # even if it exists.
        user_net = self.vpc_sg.get_network(sg_info.network_id)

        service_sg = sg_info.service_sg
        user_sgs = sg_info.user_sgs

        security_group_ids = self.args.get('security_group_ids', [])
        if security_group_ids is None:
            security_group_ids = []

        if 'security_group_ids' in self.args and set(security_group_ids) != set(user_sgs):
            if user_sgs:
                self.metadb_sg.delete_sgroups('user')
            user_sgs = security_group_ids
            if user_sgs:
                self.metadb_sg.add_user_sgroups(user_sgs)

        if not managed:
            self.desired_security_groups = user_sgs
            return

        service_sg_allow_all = False

        if user_net.default_security_group_id == '' and not user_sgs:
            service_sg_allow_all = True

        self.desired_service_sg_rules = self.sg_service_rules(host_groups, service_sg_allow_all)
        sg_hash = self.sg_calc_hash(self.desired_service_sg_rules)

        if not service_sg:
            service_sg = self.vpc_sg.create_service_security_group(
                sg_info.network_id,
                self.desired_service_sg_rules,
            ).id
            self.metadb_sg.add_service_sgroup(service_sg, sg_hash, service_sg_allow_all)
        else:
            # change condition to commented if you want to update service_sg on hash change
            # if sg_hash != sg_info.service_sg_hash:
            if sg_info.service_sg_allow_all != service_sg_allow_all:
                if service_sg_allow_all:
                    self._update_service_sg(service_sg_allow_all)
                else:
                    self.remove_allow_all_rule = True

        self.desired_security_groups = [service_sg]
        if len(user_sgs):
            self.desired_security_groups.extend(user_sgs)
        else:
            if user_net.default_security_group_id != '':
                self.desired_security_groups.append(user_net.default_security_group_id)

    def _update_service_sg(self, allow_all=False):
        sg_info = self.metadb_sg.get_sgroup_info(self.task['cid'])
        service_sg = sg_info.service_sg
        sg_hash = self.sg_calc_hash(self.desired_service_sg_rules)
        self.vpc_sg.set_security_group_rules(sg_id=service_sg, rules=self.desired_service_sg_rules)
        self.metadb_sg.update_sg_hash(service_sg, sg_hash, allow_all)
