"""
Common cluster hosts restart executor.
"""

from collections import defaultdict
from copy import deepcopy
from ...common.deploy import BaseDeployExecutor
from ...utils import build_host_group, group_host_list_by_shards, to_host_map

from ....providers.compute import ComputeApi
from ....providers.conductor import ConductorApi
from ....providers.dns import DnsApi, Record
from ....providers.juggler import JugglerApi
from ...utils import get_managed_hostname
from cloud.mdb.internal.python.compute.instances import InstanceStatus


class ClusterHostsRestartExecutor(BaseDeployExecutor):
    """
    Generic class for cluster delete executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.host_type_role = {}
        self.compute_api = ComputeApi(self.config, self.task, self.queue)
        self.dns_api = DnsApi(self.config, self.task, self.queue)
        self.juggler = JugglerApi(self.config, self.task, self.queue)
        self.conductor = ConductorApi(self.config, self.task, self.queue)

    def _classify_host_map(self, hosts):
        """
        Classify dict of hosts.
        """
        classified_hosts = defaultdict(dict)

        for host, opts in hosts.items():
            opts.update({'fqdn': host})
            for subcluster, role in self.host_type_role.items():
                if 'roles' not in opts or role in opts['roles']:
                    classified_hosts[subcluster][host] = opts

        return classified_hosts

    def _restart_host_group(self, host_group):
        """
        Stop hosts and then start them,
        actually it is ALMOST copy of BaseStopExecutor._stop_host_group + BaseStartExecutor._start_host_group
        But I don't think that I want to use multi-inheritance
        """

        # Stop
        stop_operations = []
        for host, opts in host_group.hosts.items():
            if opts['vtype'] == 'compute':
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_exists(managed_hostname)
                operation_id = self.compute_api.instance_stopped(
                    host,
                    opts['vtype_id'],
                    termination_grace_period=getattr(host_group.properties, 'termination_grace_period', None),
                )
                stop_operations.append((host, managed_hostname, operation_id))
            else:
                raise RuntimeError('Unknown vtype: {vtype} for host {host}'.format(vtype=opts['vtype'], host=host))

        for host, managed_hostname, operation_id in stop_operations:
            if operation_id:
                self.compute_api.operation_wait(operation_id)
                self.compute_api.instance_wait(host, InstanceStatus.STOPPED)

        # Start
        start_vms = []

        for host, opts in host_group.hosts.items():
            operation_id = self.compute_api.instance_running(host, opts['vtype_id'])
            start_vms.append((host, operation_id))

        for host, operation_id in start_vms:
            if operation_id:
                self.compute_api.operation_wait(operation_id)
                self.compute_api.instance_wait(host, InstanceStatus.RUNNING)
            self._create_managed_record(host, host_group)

        self._health_host_group(
            host_group,
            context_suffix="after-restart.{}".format('.'.join(host_group.hosts.keys())),
        )
        for host, opts in host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_absent(managed_hostname)

    def _create_managed_record(self, host, host_group):
        """
        Create managed and public dns records for managed hosts
        """
        managed_records = [
            Record(
                address=self.compute_api.get_instance_setup_address(host),
                record_type='AAAA',
            ),
        ]
        self.dns_api.set_records(get_managed_hostname(host, host_group.hosts[host]), managed_records)
        public_records = []
        for address, version in self.compute_api.get_instance_public_addresses(host):
            public_records.append(Record(address=address, record_type=('AAAA' if version == 6 else 'A')))
        self.dns_api.set_records(host, public_records)

    def run(self):
        # Here we get hosts, which we need to restart
        # Map them by subcluster (if any)
        # Then for each subcluster, map host by shard (if any)
        # And then - for each needed shard check quorum (alive at least n/2+1 hosts) and restart needed host
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        hosts = self._classify_host_map({host: self.args['hosts'][host] for host in self.args['host_names']})

        all_hosts = self._classify_host_map(self.args['hosts'])

        for host_type, host_map in hosts.items():
            properties = deepcopy(getattr(self.config, host_type))
            all_shards = group_host_list_by_shards(all_hosts[host_type].values())

            shards = group_host_list_by_shards(host_map.values())

            # Now we need to separate hosts to groups, one host from shard in group per iteration
            while len(shards.keys()) > 0:
                # hdict - dict{shard: host_for_restart}
                hdict = {}
                for shard in list(shards.keys()):
                    hdict[shard] = shards[shard].pop()
                    if len(shards[shard]) == 0:
                        shards.pop(shard, None)

                    # other_hosts - other hosts in shard, so we can check their health
                    other_hosts = [h for h in all_shards[shard] if h['fqdn'] != hdict[shard]['fqdn']]

                    # Check health of hosts which are not going to restart in given shard
                    # Do not check health if there are no other hosts in given shard
                    if other_hosts:
                        self._health_host_group(
                            build_host_group(properties, to_host_map(other_hosts)),
                            context_suffix="before-restart-{}-{}".format(shard, hdict[shard]['fqdn']),
                            allow_fail_hosts=len(all_shards[shard]) / 2 - 1,  # We need at least n/2+1 alive hosts
                        )

                # restart hosts
                self._restart_host_group(
                    build_host_group(properties, to_host_map(hdict.values())),
                )

        self.mlock.unlock_cluster()
