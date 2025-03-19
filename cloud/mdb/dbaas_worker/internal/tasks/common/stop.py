"""
Common stop executor
"""

from ...providers.aws.ec2 import EC2
from ...providers.aws.route53 import Route53
from ...providers.compute import ComputeApi
from ...providers.conductor import ConductorApi
from ...providers.dns import DnsApi
from ...providers.eds import EdsApi
from ...providers.juggler import JugglerApi
from ..utils import get_managed_hostname, get_private_hostname
from .base import BaseExecutor
from cloud.mdb.internal.python.compute.instances import InstanceStatus

STOP_DOWNTIME_DURATION = 24 * 3600  # 1 day


class BaseStopExecutor(BaseExecutor):
    """
    Generic class for cluster stop executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.compute_api = ComputeApi(self.config, self.task, self.queue)
        self.ec2 = EC2(self.config, self.task, self.queue)
        self.route53 = Route53(self.config, self.task, self.queue)
        self.dns_api = DnsApi(self.config, self.task, self.queue)
        self.eds_api = EdsApi(self.config, self.task, self.queue)
        self.juggler = JugglerApi(self.config, self.task, self.queue)
        self.conductor = ConductorApi(self.config, self.task, self.queue)

    def _stop_host_group(self, host_group):
        """
        Stop hosts in compute and drop dns records
        """
        stop_operations = []
        aws_stopping_hosts = []
        for host, opts in host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            if opts['vtype'] == 'compute':
                self.juggler.downtime_exists(managed_hostname, duration=STOP_DOWNTIME_DURATION)
                operation_id = self.compute_api.instance_stopped(
                    host,
                    opts['vtype_id'],
                    termination_grace_period=getattr(host_group.properties, 'termination_grace_period', None),
                )
                stop_operations.append((host, managed_hostname, operation_id))
            elif opts['vtype'] == 'aws':
                self.ec2.instance_stopped(instance_id=opts['vtype_id'], region_name=opts['region_name'])
                aws_stopping_hosts.append((host, opts))
            else:
                raise RuntimeError('Unknown vtype: {vtype} for host {host}'.format(vtype=opts['vtype'], host=host))

        for host, managed_hostname, operation_id in stop_operations:
            if operation_id:
                self.compute_api.operation_wait(operation_id)
                self.compute_api.instance_wait(host, InstanceStatus.STOPPED)
            self.conductor.host_absent(managed_hostname)
            self.eds_api.host_unregister(managed_hostname)
            self.dns_api.set_records(host, [])
            self.dns_api.set_records(managed_hostname, [])

        for host, opts in aws_stopping_hosts:
            self.ec2.instance_wait_until_stopped(instance_id=opts['vtype_id'], region_name=opts['region_name'])
            self.route53.set_records_in_public_zone(host, [])
            self.route53.set_records_in_public_zone(get_private_hostname(host, opts), [])
