"""
DBaaS Internal API MySQL host methods
"""
from typing import cast, Dict

from .constants import MY_CLUSTER_TYPE
from .host_pillar import MysqlHostPillar
from .pillar import get_cluster_pillar
from .traits import HostReplicaType, HostRole, MySQLOperations, MySQLRoles, ServiceType
from .utils import get_cluster_version
from .validation import validate_mysql_repl_source
from ...core.exceptions import (
    BatchHostCreationNotImplementedError,
    BatchHostDeletionNotImplementedError,
    BatchHostModifyNotImplementedError,
    DbaasClientError,
    HostHasActiveReplicsError,
    HostNotExistsError,
    NoChangesError,
)
from ...health import MDBH
from ...health.health import HealthInfo, ServiceRole, ServiceReplicaType
from ...utils import metadb
from ...utils.helpers import first_value
from ...utils.host import create_host, delete_host, get_host_objects, get_hosts, modify_host
from ...utils.metadata import ModifyClusterHostsMetadata
from ...utils.network import validate_host_public_ip
from ...utils.operation_creator import create_finished_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import HostStatus, MetadbHostType
from ...utils.validation import validate_hosts_count, validate_repl_source, assign_public_ip_changed


def mysql_host_extra_formatter(host: Dict, cluster: Dict) -> Dict:
    """
    Extra formatter for MySQL host
    """
    pillar = MysqlHostPillar(metadb.get_fqdn_pillar(host['fqdn']))
    cluster_pillar = get_cluster_pillar(cluster)
    version = get_cluster_version(cluster['cid'], cluster_pillar)
    return pillar.format_options(version)


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE)
def create_mysql_host(cluster, host_specs, **_):
    """
    Adds MySQL host.
    """
    if not host_specs:
        raise DbaasClientError('No hosts to add are specified')

    if len(host_specs) > 1:
        raise BatchHostCreationNotImplementedError()

    host_spec = host_specs[0]
    geo = host_spec['zone_id']

    hosts = get_hosts(cluster['cid'])
    opts = first_value(hosts)
    # TODO: write test for adding a lot of hosts to cluster
    validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=MySQLRoles.mysql.value,  # pylint: disable=no-member
        resource_preset_id=opts['flavor_name'],
        disk_type_id=opts['disk_type_id'],
        hosts_count=len(hosts) + 1,
    )

    pillar = get_cluster_pillar(cluster)
    pillar.max_server_id = (pillar.max_server_id or len(hosts)) + 1
    metadb.update_cluster_pillar(cluster['cid'], pillar)

    host_pillar = MysqlHostPillar({})
    host_pillar.server_id = pillar.max_server_id

    if 'replication_source' in host_spec:
        validate_repl_source(_repl_source_for_hosts(hosts), {None: host_spec['replication_source']})
        host_pillar.repl_source = host_spec['replication_source']

    if 'backup_priority' in host_spec:
        host_pillar.backup_priority = host_spec['backup_priority']

    if 'priority' in host_spec:
        host_pillar.priority = host_spec['priority']

    return create_host(
        MY_CLUSTER_TYPE,
        cluster,
        geo,
        host_spec.get('subnet_id'),
        assign_public_ip=host_spec['assign_public_ip'],
        pillar=host_pillar.as_dict(),
        args={
            'zk_hosts': pillar.zk_hosts,
        },
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_MODIFY)
def modify_mysql_host(cluster, update_host_specs, **_):
    """
    Modifies MySQL host.
    """

    has_changed = False
    if not update_host_specs:
        raise NoChangesError()

    if len(update_host_specs) > 1:
        raise BatchHostModifyNotImplementedError()

    host_spec = update_host_specs[0]

    host_name = host_spec['host_name']
    hosts = get_hosts(cluster['cid'])
    if host_name not in hosts:
        raise HostNotExistsError(host_name)

    host_pillar = MysqlHostPillar(metadb.get_fqdn_pillar(fqdn=host_name))
    # need to run metadata on all other hosts
    ha_status_changed = False
    public_ip_changed = False
    # need to run service operation on host
    has_changed = False
    # only metadb changes, no operation on hosts
    metadb_changed = False

    pillar = get_cluster_pillar(cluster)

    if 'replication_source' in host_spec:
        repl_source = host_spec.get('replication_source')
        repl_sources = _repl_source_for_hosts(hosts)
        # changed config
        new_repl_sources = repl_sources.copy()
        new_repl_sources[host_name] = repl_source
        if validate_repl_source(repl_sources, {host_name: repl_source}) and validate_mysql_repl_source(
            pillar, new_repl_sources
        ):
            ha_status_changed = True
            has_changed = True
            host_pillar.repl_source = repl_source

    if 'backup_priority' in host_spec:
        backup_priority = host_spec.get('backup_priority') or 0
        if host_pillar.backup_priority != backup_priority:
            metadb_changed = True
            host_pillar.backup_priority = backup_priority

    if 'priority' in host_spec:
        priority = host_spec.get('priority') or 0
        if host_pillar.priority != priority:
            metadb_changed = True
            has_changed = True
            host_pillar.priority = priority

    if 'assign_public_ip' in host_spec:
        changed = assign_public_ip_changed(host_name, host_spec['assign_public_ip'])
        has_changed |= changed
        if changed:
            public_ip_changed = True
            if host_spec['assign_public_ip']:
                validate_host_public_ip(host_name, host_spec['assign_public_ip'])
            metadb.update_host(host_name, cluster['cid'], assign_public_ip=host_spec['assign_public_ip'])

    if not has_changed and not metadb_changed:
        raise NoChangesError()

    metadb.update_host_pillar(cluster['cid'], host_name, host_pillar.as_dict())

    if has_changed:
        return modify_host(
            MY_CLUSTER_TYPE,
            cluster,
            host_name,
            args={
                'ha_status_changed': ha_status_changed,
                'include-metadata': public_ip_changed,
                'zk_hosts': pillar.zk_hosts,
            },
        )

    if metadb_changed:
        return create_finished_operation(
            operation_type=MySQLOperations.host_modify,
            metadata=ModifyClusterHostsMetadata(host_names=[host_name]),
            cid=cluster['cid'],
        )

    raise NoChangesError()


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_DELETE)
def delete_mysql_host(cluster, host_names, **_):
    """
    Deletes MySQL host.
    """
    if not host_names:
        raise DbaasClientError('No hosts to delete are specified')

    if len(host_names) > 1:
        raise BatchHostDeletionNotImplementedError()

    host_name = host_names[0]
    hosts = cast(Dict[str, MetadbHostType], get_hosts(cluster['cid']))
    host = hosts.get(host_name)
    if host is None:
        raise HostNotExistsError(host_name)

    # TODO: write test for deleting last host in cluster
    validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=MySQLRoles.mysql.value,  # pylint: disable=no-member
        resource_preset_id=host['flavor_name'],
        disk_type_id=host['disk_type_id'],
        hosts_count=len(hosts) - 1,
    )

    pillar = get_cluster_pillar(cluster)
    new_repl_sources = _repl_source_for_hosts(hosts)

    for fqdn, repl_source in new_repl_sources.items():
        if host_name == repl_source:
            raise HostHasActiveReplicsError(host_name, slaves=fqdn)

    del new_repl_sources[host_name]

    validate_mysql_repl_source(pillar=pillar, hosts_repl_src=new_repl_sources)

    return delete_host(
        MY_CLUSTER_TYPE,
        cluster,
        host_name,
        args={
            'zk_hosts': pillar.zk_hosts,
        },
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
def list_mysql_hosts(cluster, **_):
    """
    Returns MySQL hosts.
    """
    hosts = get_host_objects(cluster, extra_formatter=mysql_host_extra_formatter)
    cid = str(cluster['cid'])

    fqdns = [host['name'] for host in hosts]
    healthinfo = MDBH.health_by_fqdns(fqdns)

    for host in hosts:
        host.update(diagnose_host(cid, host['name'], healthinfo))
    return {'hosts': hosts}


_HOST_REPLICA_TYPE_FROM_SERVICE_ROLE = {
    ServiceReplicaType.unknown: HostReplicaType.unknown,
    ServiceReplicaType.a_sync: HostReplicaType.a_sync,
    ServiceReplicaType.quorum: HostReplicaType.quorum,
}


def replica_type_from_service_to_host(replica_type: ServiceReplicaType) -> HostReplicaType:
    """
    Converts host replica type from service replica type.
    """
    return _HOST_REPLICA_TYPE_FROM_SERVICE_ROLE.get(replica_type, HostReplicaType.unknown)


def get_ha_host_pillars(cid: str) -> Dict[str, MysqlHostPillar]:
    """
    Get pillars for HA hosts
    """
    hosts = get_hosts(cid)
    host_pillars = _get_host_pillars(hosts)
    ha_hosts = {}
    for fqdn, pillar in host_pillars.items():
        if not pillar.repl_source:
            ha_hosts[fqdn] = pillar

    return ha_hosts


def _get_host_pillars(hosts) -> Dict[str, MysqlHostPillar]:
    host_pillars = {}  # type: Dict[str, MysqlHostPillar]
    for fqdn in hosts:
        host_pillars[fqdn] = MysqlHostPillar(metadb.get_fqdn_pillar(fqdn=fqdn))

    return host_pillars


def _repl_source_for_hosts(hosts) -> Dict[str, str]:
    host_pillars = _get_host_pillars(hosts)
    return {host: pillar.repl_source for host, pillar in host_pillars.items()}


def diagnose_host(cid: str, hostname: str, healthinfo: HealthInfo) -> dict:
    """
    Enriches hosts with their health info.
    """

    services = []
    role = HostRole.unknown
    replica_type = HostReplicaType.unknown
    replica_upstream = None
    replica_lag = None
    system = {}

    hoststatus = HostStatus.unknown
    hosthealth = healthinfo.host(hostname, cid)
    if hosthealth is not None:
        hoststatus = hosthealth.status()
        for shealth in hosthealth.services():
            if shealth.role == ServiceRole.master:
                role = HostRole.master
            elif shealth.role == ServiceRole.replica:
                role = HostRole.replica
                replica_type = replica_type_from_service_to_host(shealth.replicaType)
                replica_upstream = shealth.replicaUpstream
                replica_lag = shealth.replicaLag
            services.append(
                {
                    'type': ServiceType.mysql,
                    'health': shealth.status,
                }
            )

        system = hosthealth.system_dict()

    hostdict = {
        'name': hostname,
        'role': role,
        'replica_type': replica_type,
        'replica_upstream': replica_upstream,
        'replica_lag': replica_lag,
        'health': hoststatus,
        'services': services,
    }
    if system:
        hostdict['system'] = system

    return hostdict
