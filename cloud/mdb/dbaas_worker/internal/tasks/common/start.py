"""
Common start executor
"""

from ...providers.aws.ec2 import EC2
from ...providers.aws.route53 import Route53
from ...providers.compute import ComputeApi
from ...providers.conductor import ConductorApi
from ...providers.dns import DnsApi, Record
from ...providers.eds import EdsApi
from ...providers.juggler import JugglerApi
from ...utils import get_eds_root, get_first_value
from ..utils import get_managed_hostname, get_private_hostname
from .base import BaseExecutor
from cloud.mdb.internal.python.compute.instances import InstanceStatus


class BaseStartExecutor(BaseExecutor):
    """
    Base class for start executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.compute_api = ComputeApi(self.config, self.task, self.queue)
        self.ec2 = EC2(self.config, self.task, self.queue)
        self.route53 = Route53(self.config, self.task, self.queue)
        self.dns_api = DnsApi(self.config, self.task, self.queue)
        self.eds = EdsApi(self.config, self.task, self.queue)
        self.juggler = JugglerApi(self.config, self.task, self.queue)
        self.conductor = ConductorApi(self.config, self.task, self.queue)

    def _start_host_group(self, host_group):
        """
        Start hosts in compute and create dns records
        """

        create_vms = []
        aws_starting_vms = []
        group_id = getattr(host_group.properties, 'conductor_group_id', self.task['cid'])

        for host, opts in host_group.hosts.items():
            if opts['vtype'] == 'compute':
                operation_id = self.compute_api.instance_running(host, opts['vtype_id'])
                create_vms.append((host, operation_id))
            elif opts['vtype'] == 'aws':
                self.ec2.instance_running(instance_id=opts['vtype_id'], region_name=opts['region_name'])
                aws_starting_vms.append((host, opts))
            else:
                raise RuntimeError('Unknown vtype: {vtype} for host {host}'.format(vtype=opts['vtype'], host=host))

        for host, operation_id in create_vms:
            if operation_id:
                self.compute_api.operation_wait(operation_id)
                self.compute_api.instance_wait(host, InstanceStatus.RUNNING)
            self._create_managed_record(host, host_group)

        for host, opts in host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_exists(managed_hostname)
            self.conductor.host_exists(managed_hostname, opts['geo'], group_id)
            self.eds.host_register(
                managed_hostname,
                group_id,
                get_eds_root(host_group.properties, get_first_value(host_group.hosts)),
            )

        for host, opts in aws_starting_vms:
            self.ec2.instance_wait_until_running(instance_id=opts['vtype_id'], region_name=opts['region_name'])
            addresses = self.ec2.get_instance_addresses(opts['vtype_id'], opts['region_name'])
            self.route53.set_records_in_public_zone(host, addresses.public_records())
            self.route53.set_records_in_public_zone(get_private_hostname(host, opts), addresses.private_records())

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
