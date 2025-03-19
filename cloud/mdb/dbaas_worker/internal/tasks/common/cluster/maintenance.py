"""
Common cluster maintenance executor.
"""

from ....providers.compute import ComputeApi
from ....providers.dns import DnsApi, Record
from ....providers.juggler import JugglerApi
from ....providers.tls_cert import TLSCert
from ..deploy import BaseDeployExecutor
from ...utils import issue_tls_wrapper, HostGroup, get_managed_hostname, Downtime


class ClusterMaintenanceExecutor(BaseDeployExecutor):
    """
    Base class for cluster maintenance executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.tls_cert = TLSCert(self.config, self.task, self.queue)
        self.juggler = JugglerApi(self.config, self.task, self.queue)
        self.compute_api = ComputeApi(self.config, self.task, self.queue)
        self.dns_api = DnsApi(self.config, self.task, self.queue)

    def _is_offline_maintenance(self) -> bool:
        """
        Returns true if it is maintenance for stopped cluster
        """
        # include 'intermediate' cluster state cause we got cluster status in the query which modifies it.
        return self.task['cluster_status'] in ('MAINTAINING-OFFLINE', 'MAINTAIN-OFFLINE-ERROR', 'STOPPED')

    def _start_offline_maintenance(self, host_group: HostGroup) -> None:
        """
        Set disable billing flag, start nodes and create managed DSN records
        """
        if not self._is_offline_maintenance():
            return

        self.logger.info('disable billing, cause it is offline maintenance')
        disable_ops: list[str] = []
        for host, opts in host_group.hosts.items():
            vtype = opts['vtype']
            if vtype == 'compute':
                disable_ops.append(self.compute_api.disable_billing(host))
            else:
                raise RuntimeError(f'Offline cluster maintenance does not support {vtype} {host}')
        self.compute_api.operations_wait(disable_ops)

        start_ops: list[str] = []
        for host, opts in host_group.hosts.items():
            if op_id := self.compute_api.instance_running(
                fqdn=host,
                instance_id=opts['vtype_id'],
            ):
                start_ops.append(op_id)
        if start_ops:
            self.compute_api.operations_wait(start_ops)

        for host in host_group.hosts:
            managed_records = [
                Record(
                    address=self.compute_api.get_instance_setup_address(host),
                    record_type='AAAA',
                ),
            ]
            self.dns_api.set_records(get_managed_hostname(host, host_group.hosts[host]), managed_records)

    def _stop_offline_maintenance(self, host_group: HostGroup) -> None:
        """
        Stop nodes, remove disable-billing flag, remove managed DNS records
        """
        if not self._is_offline_maintenance():
            return

        stop_ops: list[str] = []
        self.logger.info('stopping hosts, cause it is offline maintenance')
        for host, opts in host_group.hosts.items():
            vtype = opts['vtype']
            if vtype == 'compute':
                if op_id := self.compute_api.instance_stopped(
                    fqdn=host,
                    instance_id=opts['vtype_id'],
                    termination_grace_period=getattr(host_group.properties, 'termination_grace_period', None),
                ):
                    stop_ops.append(op_id)
            else:
                raise RuntimeError(f'Offline cluster maintenance does not support {vtype} {host}')
        self.compute_api.operations_wait(stop_ops)

        self.logger.info('enable billing on stopped hosts')
        enable_ops: list[str] = []
        for fqdn in host_group.hosts:
            if op_id := self.compute_api.enable_billing(fqdn):
                enable_ops.append(op_id)
        if enable_ops:
            self.compute_api.operations_wait(enable_ops)

        for host in host_group.hosts:
            self.dns_api.set_records(get_managed_hostname(host, host_group.hosts[host]), [])

    def _issue_tls(self, host_group, force_tls_certs=False):
        """
        Create TLS certificate for hosts in cluster
        """
        return issue_tls_wrapper(
            self.task['cid'], host_group, self.tls_cert, force=True, force_tls_certs=force_tls_certs
        )

    def _run_maintenance_task(self, host_group, get_order, operation_title='maintenance-hs', disable_monitoring=False):
        if self.args.get('update_tls', False):
            force_tls_certs = self.args.get('force_tls_certs', False)
            self._issue_tls(host_group, force_tls_certs=force_tls_certs)

        for host in host_group.hosts:
            host_group.hosts[host]['deploy'] = {'pillar': {}}
            if self.args.get('restart', False):
                host_group.hosts[host]['deploy']['pillar']['service-restart'] = True
        order = get_order()
        if disable_monitoring:
            with Downtime(self.juggler, host_group, self.task['timeout']):
                self._highstate_host_group(
                    host_group,
                    title=operation_title,
                    order=order,
                )
        else:
            self._highstate_host_group(
                host_group,
                title=operation_title,
                order=order,
            )
