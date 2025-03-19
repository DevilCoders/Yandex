"""
DBaaS Internal API mdb-health functions
"""

import json
import logging
import time
from enum import Enum, unique
from typing import Any, Dict, List, NamedTuple, Optional

import opentracing
import requests

from dbaas_common import tracing

from ..utils.logs import log_warn
from ..utils.request_context import get_x_request_id
from ..utils.types import ClusterHealth, HostStatus


class MDBHClientNotInitialized(Exception):
    """
    Exception when client to MDB Health was not initialized.
    """


class InvalidHealthData(Exception):
    """
    Exception when received malformed response from MDB Health.
    """


@unique
class ServiceStatus(Enum):
    """
    Possible service statuses.
    """

    unknown = 'ServiceStatusUnknown'
    alive = 'ServiceStatusAlive'
    dead = 'ServiceStatusDead'


_SS_FROM_STRING = {
    'Alive': ServiceStatus.alive,
    'Dead': ServiceStatus.dead,
    'Unknown': ServiceStatus.unknown,
}


def service_status_from_mdbh(status: str) -> ServiceStatus:
    """
    Converts service status from mdbh.
    """
    return _SS_FROM_STRING.get(status, ServiceStatus.unknown)


@unique
class ServiceRole(Enum):
    """
    Possible service statuses.
    """

    unknown = 'ServiceRoleUnknown'
    master = 'ServiceRoleMaster'
    replica = 'ServiceRoleReplica'


_SR_FROM_STRING = {
    'Master': ServiceRole.master,
    'Replica': ServiceRole.replica,
    'Unknown': ServiceRole.unknown,
}


def service_role_from_mdbh(role: str) -> ServiceRole:
    """
    Converts service role from mdbh.
    """
    return _SR_FROM_STRING.get(role, ServiceRole.unknown)


_HH_FROM_STRING = {
    'Alive': HostStatus.alive,
    'Unknown': HostStatus.unknown,
    'Dead': HostStatus.dead,
    'Degraded': HostStatus.degraded,
}


def host_status_from_mdbh(status: str) -> HostStatus:
    """
    Converts cluster health status from mdbh.
    """
    return _HH_FROM_STRING.get(status, HostStatus.unknown)


_CH_FROM_STRING = {
    'Alive': ClusterHealth.alive,
    'Unknown': ClusterHealth.unknown,
    'Dead': ClusterHealth.dead,
    'Degraded': ClusterHealth.degraded,
}


def cluster_health_from_mdbh(status: str) -> ClusterHealth:
    """
    Converts cluster health status from mdbh.
    """
    return _CH_FROM_STRING.get(status, ClusterHealth.unknown)


@unique
class ServiceReplicaType(Enum):
    """
    Possible service statuses.
    """

    unknown = 'ServiceReplicaUnknown'
    a_sync = 'ServiceReplicaAsync'
    sync = 'ServiceReplicaSync'
    quorum = 'ServiceReplicaQuorum'


_SRT_FROM_STRING = {
    'Unknown': ServiceReplicaType.unknown,
    'Async': ServiceReplicaType.a_sync,
    'Sync': ServiceReplicaType.sync,
    'Quorum': ServiceReplicaType.quorum,
}


def service_replica_type_from_mdbh(replica_type: str) -> ServiceReplicaType:
    """
    Converts service replica type from mdbh.
    """
    return _SRT_FROM_STRING.get(replica_type, ServiceReplicaType.unknown)


ServiceHealth = NamedTuple(
    'ServiceHealth',
    [
        ('cid', str),
        ('fqdn', str),
        ('name', str),
        ('status', ServiceStatus),
        ('role', ServiceRole),
        ('replicaType', ServiceReplicaType),
        ('replicaUpstream', str),
        ('replicaLag', int),
    ],
)


class CPUMetric(NamedTuple):
    timestamp: int
    used: float


class MemoryMetric(NamedTuple):
    timestamp: int
    used: int
    total: int


class DiskMetric(NamedTuple):
    timestamp: int
    used: int
    total: int


class SystemMetrics(NamedTuple):
    cpu: Optional[CPUMetric]
    memory: Optional[MemoryMetric]
    disk: Optional[DiskMetric]


class HostHealth:
    """
    Holds health data for a single host
    """

    def __init__(self, r: Dict[str, Any]) -> None:
        try:
            self._cid = r['cid']
            self._fqdn = r['fqdn']
            self._status = HostStatus.unknown
            if 'status' in r:
                self._status = host_status_from_mdbh(r['status'])

            services = []
            for service in r['services']:
                services.append(
                    ServiceHealth(
                        cid=self._cid,
                        fqdn=self._fqdn,
                        name=service['name'],
                        status=service_status_from_mdbh(service['status']),
                        role=service_role_from_mdbh(service.get('role')),
                        replicaType=service_replica_type_from_mdbh(service.get('replicatype')),
                        replicaUpstream=service.get('replica_upstream'),
                        replicaLag=service.get('replica_lag'),
                    )
                )

            self._services = services

            system = None
            if 'system' in r:
                rs = r['system']

                cpu = None
                if 'cpu' in rs:
                    cpu = CPUMetric(
                        timestamp=rs['cpu']['timestamp'],
                        used=rs['cpu']['used'],
                    )

                memory = None
                if 'mem' in rs:
                    memory = MemoryMetric(
                        timestamp=rs['mem']['timestamp'],
                        used=rs['mem']['used'],
                        total=rs['mem']['total'],
                    )

                disk = None
                if 'disk' in rs:
                    disk = DiskMetric(
                        timestamp=rs['disk']['timestamp'],
                        used=rs['disk']['used'],
                        total=rs['disk']['total'],
                    )

                if cpu is not None or memory is not None or disk is not None:
                    system = SystemMetrics(
                        cpu=cpu,
                        memory=memory,
                        disk=disk,
                    )

            self._system = system
        except (KeyError, TypeError) as exc:
            raise InvalidHealthData from exc

    def __eq__(self, rhs):
        if not isinstance(rhs, self.__class__):
            return False

        return self.cid() == rhs.cid() and self.fqdn() == rhs.fqdn() and self.services() == rhs.services()

    def __ne__(self, rhs):
        return not self.__eq__(rhs)

    def fqdn(self) -> str:
        """
        Returns fqdn for this host
        """

        return self._fqdn

    def cid(self) -> str:
        """
        Returns cluster id for this host
        """

        return self._cid

    def services(self) -> List[ServiceHealth]:
        """
        Returns list of services for this host
        """

        return self._services

    def status(self) -> HostStatus:
        """
        Returns host health status
        """
        return self._status

    def system(self) -> Optional[SystemMetrics]:
        """
        Returns host system metrics
        """

        return self._system

    def system_dict(self) -> dict:
        system = {}

        if self._system is not None:
            if cpu := self._system.cpu:
                system['cpu'] = {
                    'timestamp': cpu.timestamp,
                    'used': cpu.used,
                }

            if memory := self._system.memory:
                system['memory'] = {
                    'timestamp': memory.timestamp,
                    'used': memory.used,
                    'total': memory.total,
                }

            if disk := self._system.disk:
                system['disk'] = {
                    'timestamp': disk.timestamp,
                    'used': disk.used,
                    'total': disk.total,
                }

        return system


class HealthInfo:
    """
    Holds health data requested from MDB Health
    """

    def __init__(self, hosts_dict: List[dict]) -> None:
        hosts = {}  # type: Dict[str, HostHealth]

        for hdict in hosts_dict:
            host = HostHealth(hdict)
            hosts[host.fqdn()] = host

        self._hosts = hosts

    def __eq__(self, rhs):
        if not isinstance(rhs, self.__class__):
            return False

        return self.hosts() == rhs.hosts()

    def __ne__(self, rhs):
        return not self.__eq__(rhs)

    def hosts(self) -> Dict[str, HostHealth]:
        """
        Returns list of hosts
        """

        return self._hosts

    def host(self, fqdn: str, cid: str) -> Optional[HostHealth]:
        """
        Return health info for a specific host checking if cid is correct
        """

        hosthealth = self._hosts.get(fqdn, None)
        if hosthealth is None:
            return None

        if hosthealth.cid() != cid:
            log_warn(
                'Expected cid %r for fqdn %r but mdbhealth thinks its %r cid', cid, hosthealth.fqdn(), hosthealth.cid()
            )
            return None

        return hosthealth


class MDBHealth:
    """
    Provides API to MDB Health
    """

    def __init__(self) -> None:
        self._provider = None  # type: Optional[MDBHealthProviderHTTP]
        self._logger = None  # type: Optional[logging.Logger]

    def init_mdbhealth(self, provider, logger_name: str) -> None:
        """
        Initializes API from config
        """

        self._provider = provider
        self._logger = logging.getLogger(logger_name)

    def health_by_cid(self, cid: str) -> ClusterHealth:
        """
        Retrieves health of specified cid
        """

        if self._provider is None or self._logger is None:
            raise MDBHClientNotInitialized

        try:
            return self._provider.get_cluster_health(cid, self._logger)
        except IOError as exc:
            # Network error
            self._logger.warning('mdb-health: error while talking to mdb-health: %s', exc)
        except (ValueError, KeyError, InvalidHealthData) as exc:
            # Invalid JSON or missing keys
            self._logger.error('mdb-health: received bad health response %s', exc)

        return ClusterHealth.unknown

    def health_by_fqdns(self, fqdns: List[str]) -> HealthInfo:
        """
        Retrieves health of specified fqdns
        """

        if self._provider is None or self._logger is None:
            raise MDBHClientNotInitialized

        try:
            return self._provider.get_hosts_health(fqdns, self._logger)
        except IOError as exc:
            # Network error
            self._logger.warning('mdb-health: error while talking to mdb-health: %s', exc)
        except (ValueError, KeyError, InvalidHealthData) as exc:
            # Invalid JSON or missing keys
            self._logger.error('mdb-health: received bad health response %s', exc)

        return HealthInfo([])


class MDBHealthProviderHTTP:
    """
    Provides access to MDB Health service
    """

    def __init__(self, config) -> None:
        self._list_hosts_health_url = "%s/v1/listhostshealth" % config['url']
        self._get_cluster_health_url = "%s/v1/clusterhealth" % config['url']
        self._connect_timeout = config['connect_timeout']
        self._read_timeout = config['read_timeout']
        self._session = requests.Session()
        self._session.verify = config['ca_certs']

    @tracing.trace('MDB Health GetCluster')
    def get_cluster_health(self, cid: str, logger: logging.Logger) -> ClusterHealth:
        """
        Retrieves health of specified cid from MDB Health service
        """
        tracing.set_tag('cluster.id', cid)

        x_req_id = get_x_request_id()
        start_at = time.time()
        try:
            resp = self._session.get(
                self._get_cluster_health_url,
                params={'cid': cid},
                headers={
                    'X-Request-Id': x_req_id,
                    'Content-Type': 'application/json',
                },
                timeout=(self._connect_timeout, self._read_timeout),
            )
        except IOError:
            tracing.set_tag(opentracing.tags.ERROR, True)
            logger.exception('mdb-health: failed to retrieve cluster health')
            return ClusterHealth.unknown

        proc_time = (time.time() - start_at) * 1000
        logger.debug(
            'mdb-health: request, X-Request-Id: %s, cid: %s, process time: %dmsec, code: %d',
            x_req_id,
            cid,
            proc_time,
            resp.status_code,
        )

        if resp.status_code != 200:
            tracing.set_tag(opentracing.tags.ERROR, True)
            logger.error('mdb-health: failed to retrieve cluster health status: %d %s', resp.status_code, resp.reason)
            return ClusterHealth.unknown

        resp_status = resp.json()['status']
        return cluster_health_from_mdbh(resp_status)

    @tracing.trace('MDB Health GetHosts')
    def get_hosts_health(self, fqdns: List[str], logger: logging.Logger) -> HealthInfo:
        """
        Retrieves health of specified fqdns from specified URL
        """
        tracing.set_tag('cluster.fqdns', fqdns)

        x_req_id = get_x_request_id()
        start_at = time.time()
        try:
            resp = self._session.post(
                self._list_hosts_health_url,
                data=bytes(
                    json.dumps(
                        {
                            'hosts': fqdns,
                        }
                    ),
                    'utf8',
                ),
                headers={
                    'X-Request-Id': x_req_id,
                    'Content-Type': 'application/json',
                },
                timeout=(self._connect_timeout, self._read_timeout),
            )
        except IOError:
            tracing.set_tag(opentracing.tags.ERROR, True)
            logger.exception('mdb-health: failed to retrieve host health')
            return HealthInfo([])

        proc_time = (time.time() - start_at) * 1000
        logger.debug(
            'mdb-health: request, X-Request-Id: %s, host count: %d, process time: %dmsec',
            x_req_id,
            len(fqdns),
            proc_time,
        )

        if resp.status_code != 200:
            tracing.set_tag(opentracing.tags.ERROR, True)
            logger.error('mdb-health: failed to retrieve hosts status: %d %s', resp.status_code, resp.reason)
            return HealthInfo([])

        resp_hosts = resp.json()['hosts']
        return HealthInfo(resp_hosts)
