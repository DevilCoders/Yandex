"""
Host health checker
"""

import time

from dbaas_common import tracing

from ..exceptions import ExposedException
from ..tasks.utils import get_managed_hostname
from .common import BaseProvider, Change
from .juggler import JugglerApi


class HostNotHealthyError(ExposedException):
    """
    Bad cluster health error
    """


class HostHealth(BaseProvider):
    """
    HostHealth provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.juggler = JugglerApi(config, task, queue)

    @tracing.trace('HostHealth Wait')
    def wait(self, hosts, services, context_suffix, timeout=1800, allow_fail_hosts=0):
        """
        Wait until all hosts have non-crit values for all services
        """
        tracing.set_tag('cluster.host.fqdns', hosts)
        tracing.set_tag('cluster.host.services', services)

        deadline = time.time() + timeout
        with self.interruptable:
            while time.time() < deadline:
                try:
                    self.check(hosts, services, context_suffix, allow_fail_hosts)
                    return
                except HostNotHealthyError:
                    time.sleep(10)

        # Timeout passed trying one more time
        # We'll fail with reasonable exception if any host is not healty
        self.check(hosts, services, context_suffix)

    @tracing.trace('HostHealth Check')
    def check(self, hosts, services, context_suffix, allow_fail_hosts=0):
        """
        Ensure that all hosts have non-crit values for all services
        """
        tracing.set_tag('cluster.host.fqdns', hosts)
        tracing.set_tag('cluster.host.services', services)

        if self.task.get('task_args', {}).get('disable_health_check', False):
            self.add_change(Change('host-health-check', 'disabled', rollback=Change.noop_rollback))
            return

        fails = 0
        for host in hosts:
            context_key = f'host-health-{host}{context_suffix}'
            ok_from_context = self.context_get(context_key)
            if ok_from_context:
                self.add_change(Change(context_key, 'ok', context={context_key: True}, rollback=Change.noop_rollback))
                continue
            ok_from_check = True
            for service in services:
                if not self.juggler.service_not_crit(host, service):
                    fails += 1
                    if fails <= allow_fail_hosts:
                        ok_from_check = False
                        break
                    raise HostNotHealthyError('{fqdn} has CRIT for {service}'.format(fqdn=host, service=service))
            if ok_from_check:
                self.add_change(Change(context_key, 'ok', context={context_key: True}, rollback=Change.noop_rollback))

    def split(self, host_group, context_suffix):
        """
        Split host list by health status
        """
        hosts_ok = []
        hosts_unknown = []
        for host, opts in host_group.hosts.items():
            target = get_managed_hostname(host, opts)
            context_key = f'host-health-{target}{context_suffix}'
            ok_from_context = self.context_get(context_key)
            if ok_from_context:
                hosts_ok.append(host)
            else:
                hosts_unknown.append(host)
        return hosts_ok, hosts_unknown
