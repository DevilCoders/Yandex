"""
Common delete executor
"""

from typing import Callable, List

from ...providers.aws.ec2 import EC2
from ...providers.aws.route53 import Route53
from ...providers.certificator import CertificatorApi
from ...providers.compute import ComputeApi
from ...providers.conductor import ConductorApi
from ...providers.dbm import DBMApi
from ...providers.dns import DnsApi
from ...providers.eds import EdsApi
from ...providers.internal_api import InternalApi
from ...providers.juggler import JugglerApi
from ...providers.metadb_host import MetadbHost
from ...providers.placement_group import PlacementGroupProvider
from ..utils import get_managed_hostname, get_private_hostname
from .deploy import BaseDeployExecutor

DELETE_DOWNTIME_DURATION = 14 * 24 * 3600  # 2 weeks

_WaitUntilDone = Callable[[], None]


# pylint: disable=too-many-instance-attributes
class BaseDeleteExecutor(BaseDeployExecutor):
    """
    Generic class for cluster delete executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.compute_api = ComputeApi(self.config, self.task, self.queue)
        self.ec2 = EC2(self.config, self.task, self.queue)
        self.route53 = Route53(self.config, self.task, self.queue)
        self.dbm_api = DBMApi(self.config, self.task, self.queue)
        self.dns_api = DnsApi(self.config, self.task, self.queue)
        self.eds_api = EdsApi(self.config, self.task, self.queue)
        self.juggler = JugglerApi(self.config, self.task, self.queue)
        self.conductor = ConductorApi(self.config, self.task, self.queue)
        self.cert_api = CertificatorApi(config, task, queue)
        self.internal_api = InternalApi(self.config, self.task, self.queue)
        self.placement_group = PlacementGroupProvider(self.config, self.task, self.queue)
        self.metadb_host = MetadbHost(config, task, queue)

    def __delete_compute_vm(self, host: str, opts: dict, managed_hostname, full: bool) -> _WaitUntilDone:
        operation_id = self.compute_api.instance_absent(host, opts['vtype_id'])

        def wait_for_compute_vm_deleted():
            if operation_id:
                disk_type_id = opts.get('disk_type_id')
                if disk_type_id is None or disk_type_id.startswith('local-'):
                    self.compute_api.operation_wait(operation_id, timeout=50 * 60)
                else:
                    self.compute_api.operation_wait(operation_id)
                self.compute_api.instance_wait(host, self.compute_api.INSTANCE_DELETED_STATE)
            if full:
                self.dns_api.set_records(host, [])
                self.dns_api.set_records(managed_hostname, [])

        return wait_for_compute_vm_deleted

    def __delete_porto_container(self, host) -> _WaitUntilDone:
        change = self.dbm_api.container_absent(host)

        def wait_for_dbm_container_removal():
            if change:
                if change.jid:
                    self.deploy_api.wait([change.jid])
                elif change.operation_id:
                    self.dbm_api.wait_operation(change.operation_id)

        return wait_for_dbm_container_removal

    def __delete_aws_vm(self, host: str, opts: dict, full: bool) -> _WaitUntilDone:
        region_name = opts['region_name']
        terminating_ids = self.ec2.instance_absent(host, region_name)

        def wait_for_aws_vm_deleted():
            for instance_id in terminating_ids:
                self.ec2.instance_wait_until_terminated(instance_id, region_name)
            if full:
                self.route53.set_records_in_public_zone(host, [])
                self.route53.set_records_in_public_zone(get_private_hostname(host, opts), [])

        return wait_for_aws_vm_deleted

    def _delete_host_group_minimal(self, host_group):
        """
        Delete hosts from dbm/compute only
        """
        wait_for: List[Callable] = []
        host_pgid = {}

        for host, opts in host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_exists(managed_hostname, duration=DELETE_DOWNTIME_DURATION)
            if opts['vtype'] == 'compute':
                wait_for.append(self.__delete_compute_vm(host, opts, managed_hostname, full=False))
                host_info = self.metadb_host.get_host_info(host)
                if host_info:
                    placement_group_id = host_info['placement_group_id']
                    if placement_group_id:
                        host_pgid[host] = placement_group_id
            elif opts['vtype'] == 'porto':
                wait_for.append(self.__delete_porto_container(host))
            elif opts['vtype'] == 'aws':
                wait_for.append(self.__delete_aws_vm(host, opts, full=False))
            else:
                raise RuntimeError(f'Unknown vtype: {opts["vtype"]} for host {host}')

        for wait in wait_for:
            wait()

        if len(host_pgid) > 0:
            self.placement_group.remove_placement_group(host_pgid)

    def _delete_host_group_full(self, host_group):
        """
        Delete hosts from dbm/compute, dns, conductor, deploy api, and cert_api
        """
        managed_hostnames = []
        wait_for: List[Callable] = []

        for host, opts in host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_exists(managed_hostname, duration=DELETE_DOWNTIME_DURATION)
            if opts['vtype'] == 'compute':
                wait_for.append(self.__delete_compute_vm(host, opts, managed_hostname, full=True))
            elif opts['vtype'] == 'porto':
                wait_for.append(self.__delete_porto_container(host))
            elif opts['vtype'] == 'aws':
                wait_for.append(self.__delete_aws_vm(host, opts, full=True))
            else:
                raise RuntimeError('Unknown vtype: {vtype} for host {host}'.format(vtype=opts['vtype'], host=host))
            managed_hostnames.append(managed_hostname)

        for wait in wait_for:
            wait()

        for managed_hostname in managed_hostnames:
            self.conductor.host_absent(managed_hostname)
            self.eds_api.host_unregister(managed_hostname)
            # We do not need long downtime if host does not exist in conductor
            self.juggler.downtime_exists(managed_hostname)

        for host in host_group.hosts:
            self.cert_api.revoke(host)
            self.deploy_api.delete_minion(host)
        self._sync_alerts(self.task['cid'])
