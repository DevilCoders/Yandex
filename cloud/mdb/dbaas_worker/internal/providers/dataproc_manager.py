"""
Provider for dataproc-manager
"""

import os
import time
from typing import Iterable, Union, Optional
import collections.abc

import grpc

from dbaas_common import retry, tracing
from yandex.cloud.priv.dataproc.manager.v1 import manager_service_pb2, manager_service_pb2_grpc

from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException
from cloud.mdb.dbaas_worker.internal.providers.common import BaseProvider, Change


class ClusterNotFoundError(UserExposedException):
    """
    Cluster health not found error
    """


class ClusterNotHealthyError(UserExposedException):
    """
    Bad cluster health error
    """


class ClusterUnknownStatusError(UserExposedException):
    """
    Unknown cluster health status error
    """


class DataprocManager(BaseProvider):
    """
    Dataproc-manager health provider
    """

    UNKNOWN = manager_service_pb2.HEALTH_UNSPECIFIED
    ALIVE = manager_service_pb2.ALIVE
    DEAD = manager_service_pb2.DEAD
    DEGRADED = manager_service_pb2.DEGRADED
    DECOMMISSIONING = manager_service_pb2.DECOMMISSIONING
    DECOMMISSIONED = manager_service_pb2.DECOMMISSIONED

    YARN = manager_service_pb2.YARN
    HDFS = manager_service_pb2.HDFS

    def _get_ssl_creds(self):
        cert_file = self.config.dataproc_manager.cert_file
        if not cert_file or not os.path.exists(cert_file):
            return grpc.ssl_channel_credentials()
        with open(cert_file, 'rb') as file_handler:
            certs = file_handler.read()
        return grpc.ssl_channel_credentials(root_certificates=certs)

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.url = self.config.dataproc_manager.url
        self.sleep_time = self.config.dataproc_manager.sleep_time
        self.grpc_timeout = self.config.dataproc_manager.grpc_timeout
        if self.config.dataproc_manager.insecure:
            channel = grpc.insecure_channel(self.url)
        else:
            creds = self._get_ssl_creds()
            options = None
            if self.config.dataproc_manager.server_name:
                options = (('grpc.ssl_target_name_override', self.config.dataproc_manager.server_name),)
            channel = grpc.secure_channel(self.url, creds, options)
        channel = tracing.grpc_channel_tracing_interceptor(channel)
        self.stub = manager_service_pb2_grpc.DataprocManagerServiceStub(channel)

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    def decommission_hosts(self, cid, hosts, timeout, only_yarn=False):
        """
        Send decommission hosts
        """
        already_decommissioned_hosts = set(self.context_get('decommissioned_hosts') or [])
        hosts_to_decommission = set(hosts) - already_decommissioned_hosts

        request = manager_service_pb2.DecommissionRequest()
        request.cid = cid
        request.yarn_hosts.extend(hosts_to_decommission)
        if not only_yarn:
            request.hdfs_hosts.extend(hosts_to_decommission)
        request.timeout = timeout

        try:
            resp = self.stub.Decommission(request, timeout=self.grpc_timeout)
        except grpc.RpcError as rpc_error:
            code = None
            # Workaround for too general error class in gRPC
            if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # pylint: disable=E1101
                code = rpc_error.code()  # pylint: disable=E1101
            if code == grpc.StatusCode.NOT_FOUND:
                self.logger.info('Cluster decommission_hosts %s %s  - Not found', cid, self.url)
                raise ClusterNotFoundError(f'Health decommission_hosts of {cid} is not found')
            raise

        change_id = f'dataproc-manager-decommission-{"-".join(sorted(hosts))}'
        self.add_change(Change(change_id, 'initiated'))
        self.logger.info('Cluster decommission_hosts %s %s %s', cid, self.url, resp)
        return resp

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    def cluster_status(self, cid):
        """
        Get cluster status from service
        """
        req = manager_service_pb2.ClusterHealthRequest()
        req.cid = cid
        try:
            resp = self.stub.ClusterHealth(req)
        except grpc.RpcError as rpc_error:
            code = None
            # Workaround for too general error class in gRPC
            if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # pylint: disable=E1101
                code = rpc_error.code()  # pylint: disable=E1101
            if code == grpc.StatusCode.NOT_FOUND:
                self.logger.info('Cluster health %s %s  - Not found', cid, self.url)
                raise ClusterNotFoundError(f'Health status of {cid} is not found')
            raise
        self.logger.info('Cluster health %s %s %s', cid, self.url, resp.health)
        return resp

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    def decommission_status(self, cid):
        """
        Get decommission status from service
        """
        req = manager_service_pb2.DecommissionStatusRequest()
        req.cid = cid
        try:
            resp = self.stub.DecommissionStatus(req, timeout=self.grpc_timeout)
        except grpc.RpcError as rpc_error:
            code = None
            # Workaround for too general error class in gRPC
            if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # pylint: disable=E1101
                code = rpc_error.code()  # pylint: disable=E1101
            if code == grpc.StatusCode.NOT_FOUND:
                self.logger.info('Decommission status %s %s  - Not found', cid, self.url)
                raise ClusterNotFoundError(f'Decommission status of {cid} is not found')
            raise
        self.logger.info('Decommission status %s %s %s', cid, self.url, resp)
        return resp

    def cluster_wait(self, cid, timeout=600, updated_after: Optional[int] = None):
        """
        Wait until cluster became alive
        """
        deadline = time.time() + timeout
        status = None
        with self.interruptable:
            while time.time() < deadline:
                try:
                    status = self.cluster_status(cid)
                except ClusterNotFoundError:
                    time.sleep(self.sleep_time)
                    continue
                if status.health == self.ALIVE and not status.hdfs_in_safemode:
                    if not updated_after or status.update_time > updated_after:
                        return
                time.sleep(self.sleep_time)

        if status is None:
            raise ClusterNotHealthyError(f'Health status of {cid} is UNKNOWN')

        if status.health == self.ALIVE and updated_after and status.update_time <= updated_after:
            raise ClusterNotHealthyError(
                f'Cluster {cid} is Alive but this information is outdated.'
                f' Expected status to be updated after {updated_after},'
                f' but update time is {status.update_time}.'
            )

        unhealthy_services = []
        for service_status in status.service_health:
            if service_status.health == self.ALIVE:
                continue
            unhealthy_services.append(manager_service_pb2.Service.Name(service_status.service))

        if status.hdfs_in_safemode:
            unhealthy_services.append('HDFS Safe Mode')

        message = '{cid} has unhealthy services: {services}'.format(cid=cid, services=",".join(unhealthy_services))
        raise ClusterNotHealthyError(message)

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    def hosts_status(self, cid, hosts):
        """
        Get status for list of hosts
        """
        req = manager_service_pb2.HostsHealthRequest()
        req.cid = cid
        req.fqdns.extend(hosts)
        resp = self.stub.HostsHealth(req)
        self.logger.info('Hosts health %s %s %s', cid, self.url, resp)
        return resp

    def wait_for_yarn_host_before_start(self, cid: str, host: Optional[str], timeout=600):
        """
        Wait until host is removed from YARN decommission list
        """
        deadline = time.time() + timeout
        with self.interruptable:
            while time.time() < deadline:
                decommission_status = self.decommission_status(cid)
                if host not in decommission_status.yarn_requested_decommission_hosts:
                    return
                # check last (empty) decommission request
                if not host and not decommission_status.yarn_requested_decommission_hosts:
                    return
                time.sleep(self.sleep_time)
        raise ClusterNotHealthyError(
            f'Host {host} of cluster {cid} did not got out decommission list until the timeout {timeout}'
        )

    def _get_detailed_error_message(self, cid, hosts, hosts_status, statuses, service):
        hosts_fqdns = ','.join(hosts)
        statuses_name = ','.join([manager_service_pb2.Health.Name(status) for status in statuses])
        message = f'{cid} has hosts with status not equal {statuses_name}.'
        if hosts_status:
            for host in hosts_status.hosts_health:
                if service:
                    for service_health in host.service_health:
                        service_status_name = manager_service_pb2.Health.Name(service_health.health)
                        message += f'{host.fqdn}-{service}-{service_status_name}'
                else:
                    host_status_name = manager_service_pb2.Health.Name(host.health)
                    message += f'{host.fqdn}-{host_status_name}'
        else:
            message += f' Hosts: {hosts_fqdns}'
        return message

    def _add_decommission_hosts_to_context(self, decommissioned_hosts):
        self.add_change(
            Change(
                'decommissioned_hosts',
                decommissioned_hosts,
                context={'decommissioned_hosts': decommissioned_hosts},
            )
        )

    def get_first_host_with_status(
        self,
        cid: str,
        hosts: Iterable[str],
        statuses: Optional[Union[int, Iterable[int]]] = None,
        timeout: int = 600,
        service: int = None,
    ):
        """
        Get first host with ont of requested statuses
        """

        if statuses is None:
            statuses = [self.ALIVE]
        elif not isinstance(statuses, collections.abc.Iterable):
            statuses = [statuses]

        if self.DECOMMISSIONED in statuses:
            # avoid extra decommission if the task was interrupted
            already_decommissioned_hosts = set(self.context_get('decommissioned_hosts') or [])
            if already_decommissioned_hosts:
                self._add_decommission_hosts_to_context(sorted(already_decommissioned_hosts))
                hosts = set(hosts) - already_decommissioned_hosts

        if not hosts:
            return

        deadline = time.time() + timeout
        hosts_status = None

        with self.interruptable:
            while time.time() < deadline:
                hosts_status = self.hosts_status(cid, hosts)
                for host in sorted(hosts_status.hosts_health, key=lambda x: x.fqdn):
                    # if service is set wait for status only for the specified service
                    if service:
                        for service_health in host.service_health:
                            if service_health.service == service and service_health.health in statuses:
                                if self.DECOMMISSIONED in statuses:
                                    self._add_decommission_hosts_to_context([host.fqdn])
                                return host.fqdn
                    elif host.health in statuses:
                        if self.DECOMMISSIONED in statuses:
                            self._add_decommission_hosts_to_context([host.fqdn])
                        return host.fqdn

                time.sleep(self.sleep_time)

        message = self._get_detailed_error_message(cid, hosts, hosts_status, statuses, service)
        raise ClusterNotHealthyError(message)

    def hosts_wait(
        self,
        cid: str,
        hosts: Iterable[str],
        statuses: Optional[Union[int, Iterable[int]]] = None,
        timeout: int = 600,
        service: int = None,
    ):
        """
        Wait till all hosts become requested status
        """
        hosts_copy = set(hosts)
        deadline = time.time() + timeout
        while hosts_copy:
            host = self.get_first_host_with_status(cid, hosts_copy, statuses, timeout, service)
            # host can be None after task interruption (context usage)
            if not host:
                return
            hosts_copy.remove(host)
            # preserve total timeout by decreasing timeout for every new host
            timeout = deadline - time.time()  # type: ignore
        return
