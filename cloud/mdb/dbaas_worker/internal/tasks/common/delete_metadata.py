"""
Common metadata delete executor
"""

from ...providers.aws.route53 import Route53
from ...providers.conductor import ConductorApi
from ...providers.dns import DnsApi
from ...providers.eds import EdsApi
from ...providers.internal_api import InternalApi
from ...providers.juggler import JugglerApi
from ...providers.certificator import CertificatorApi
from ..utils import get_managed_hostname, get_private_hostname
from .deploy import BaseDeployExecutor


class BaseDeleteMetadataExecutor(BaseDeployExecutor):
    """
    Generic class for cluster metadata delete executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.dns_api = DnsApi(self.config, self.task, self.queue)
        self.eds_api = EdsApi(self.config, self.task, self.queue)
        self.route53 = Route53(self.config, self.task, self.queue)
        self.conductor = ConductorApi(self.config, self.task, self.queue)
        self.juggler = JugglerApi(self.config, self.task, self.queue)
        self.cert_api = CertificatorApi(config, task, queue)
        self.internal_api = InternalApi(self.config, self.task, self.queue)

    def _delete_host_group(self, host_group):
        """
        Delete hosts from dns, conductor, deploy api, and cert_api
        """
        for host, opts in host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            if opts['vtype'] == 'compute':
                self.dns_api.set_records(host, [])
                self.dns_api.set_records(managed_hostname, [])
            elif opts['vtype'] == 'aws':
                self.route53.set_records_in_public_zone(host, [])
                self.route53.set_records_in_public_zone(get_private_hostname(host, opts), [])
            self.conductor.host_absent(managed_hostname)
            self.eds_api.host_unregister(managed_hostname)
            # We do not need long downtime if host does not exist in conductor
            self.juggler.downtime_exists(managed_hostname)

            self.cert_api.revoke(host)
            self.deploy_api.delete_minion(host)
