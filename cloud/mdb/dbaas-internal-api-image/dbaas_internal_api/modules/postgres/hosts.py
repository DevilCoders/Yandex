# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL host methods
"""

from flask import current_app
from marshmallow import Schema
from typing import Dict

from .constants import MY_CLUSTER_TYPE
from .host_pillar import PostgresqlHostPillar
from .pillar import PostgresqlClusterPillar
from .traits import HostReplicaType, HostRole, PostgresqlRoles, ServiceType
from .utils import host_spec_make_pillar, get_cluster_version
from .validation import (
    validate_postgresql_host_config,
    validate_priority,
    validate_version_and_ha,
    validate_ha_host_count,
)
from ...core.exceptions import (
    BatchHostCreationNotImplementedError,
    BatchHostDeletionNotImplementedError,
    BatchHostModifyNotImplementedError,
    DbaasClientError,
    HostHasActiveReplicsError,
    HostNotExistsError,
    NoChangesError,
    PreconditionFailedError,
)
from ...core.types import Operation
from ...health import MDBH
from ...health.health import HealthInfo, ServiceReplicaType, ServiceRole
from ...utils import helpers
from ...utils import host as hostutil
from ...utils import metadb
from ...utils.host import get_host_objects, get_hosts
from ...utils.network import validate_host_public_ip
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import HostStatus
from ...utils.validation import validate_hosts_count, validate_repl_source, assign_public_ip_changed


def postgres_host_extra_formatter(host: Dict, cluster: Dict) -> Dict:
    """
    Extra formatter for PostgreSQL host
    """
    pillar = PostgresqlHostPillar(metadb.get_fqdn_pillar(host['fqdn']))
    version = get_cluster_version(cluster['cid'])
    return pillar.format_options(version)


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE)
def create_postgresql_host(cluster, host_specs, **_):
    """
    Adds PostgreSQL host.
    """
    if not host_specs:
        raise DbaasClientError('No hosts to add are specified')

    if len(host_specs) > 1:
        raise BatchHostCreationNotImplementedError()

    host_spec = host_specs[0]
    geo = host_spec['zone_id']

    hosts = get_hosts(cluster['cid'])
    opts = helpers.first_value(hosts)
    # TODO: write test for adding a lot of hosts to cluster
    validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=PostgresqlRoles.postgresql.value,  # pylint: disable=no-member
        resource_preset_id=opts['flavor_name'],
        disk_type_id=opts['disk_type_id'],
        hosts_count=len(hosts) + 1,
    )
    host_count = len(hosts) if 'replication_source' in host_spec else len(hosts) + 1
    hosts_with_repl_source = {host: _repl_source_for_host(host) for host in hosts}
    validate_ha_host_count(hosts_with_repl_source, host_count)

    validate_version_and_ha(cluster, host_spec, host_spec.get('replication_source'))

    if 'priority' not in host_spec:
        host_spec['priority'] = current_app.config['ZONE_ID_TO_PRIORITY'].get(host_spec['zone_id'], 0)

    if 'replication_source' in host_spec:
        validate_repl_source(hosts_with_repl_source, {None: host_spec['replication_source']})

    host_pillar = host_spec_make_pillar(host_spec).as_dict()

    return hostutil.create_host(
        MY_CLUSTER_TYPE,
        cluster,
        geo,
        host_spec.get('subnet_id'),
        assign_public_ip=host_spec['assign_public_ip'],
        pillar=helpers.remove_none_dict(host_pillar),
        args={},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_MODIFY)
def modify_postgresql_host(cluster: Dict, update_host_specs: Dict, _schema: Schema, **_) -> Operation:
    """
    Modifies PostgreSQL host.
    """

    changes = False
    restart = False
    if not update_host_specs:
        raise NoChangesError()

    if len(update_host_specs) > 1:
        raise BatchHostModifyNotImplementedError()

    host_spec = update_host_specs[0]

    host_name = host_spec['host_name']
    hosts = hostutil.get_hosts(cluster['cid'])
    if host_name not in hosts:
        raise HostNotExistsError(host_name)

    hosts_with_repl_source = {host: _repl_source_for_host(host) for host in hosts}
    host_count = len(hosts) if 'replication_source' in host_spec else len(hosts) + 1
    validate_ha_host_count(hosts_with_repl_source, host_count)
    if 'replication_source' in host_spec:
        changes = validate_repl_source(hosts_with_repl_source, {host_name: host_spec['replication_source']}) or changes

    if 'priority' in host_spec:
        changes = validate_priority(host_name, host_spec) or changes

    if 'config_spec' in host_spec:
        changes = validate_postgresql_host_config(host_name, host_spec, cluster) or changes

    public_ip_changed = False
    if 'assign_public_ip' in host_spec:
        changed = assign_public_ip_changed(host_name, host_spec['assign_public_ip'])
        changes |= changed
        if changed:
            public_ip_changed = True
            validate_host_public_ip(host_name, host_spec['assign_public_ip'])
            metadb.update_host(host_name, cluster['cid'], assign_public_ip=host_spec['assign_public_ip'])

    if not changes:
        raise NoChangesError()
    if _schema.context.get('restart'):
        restart = True

    cluster_pillar = PostgresqlClusterPillar(cluster['value'])

    return hostutil.modify_host(
        MY_CLUSTER_TYPE,
        cluster,
        host_name,
        {
            'restart': restart,
            'pillar': host_spec_make_pillar(host_spec).as_dict(),
            'include-metadata': public_ip_changed,
            'zk_hosts': cluster_pillar.pgsync.get_zk_hosts_as_str(),
        },
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_DELETE)
def delete_postgresql_host(cluster, host_names, **_):
    """
    Deletes PostgreSQL host.
    """
    if not host_names:
        raise DbaasClientError('No hosts to delete are specified')

    if len(host_names) > 1:
        raise BatchHostDeletionNotImplementedError()

    host_name = host_names[0]
    hosts = get_hosts(cluster['cid'])
    host = hosts.get(host_name)
    if host is None:
        raise HostNotExistsError(host_name)

    if len(hosts) <= 1:
        raise PreconditionFailedError('Last PostgreSQL host cannot be removed')

    # TODO: write test for deleting last host in cluster
    validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=PostgresqlRoles.postgresql.value,  # pylint: disable=no-member
        resource_preset_id=host['flavor_name'],
        disk_type_id=host['disk_type_id'],
        hosts_count=len(hosts) - 1,
    )

    for host in hosts:
        host_pillar = PostgresqlHostPillar(metadb.get_fqdn_pillar(fqdn=host))
        # We assume that we have pillar for this fqdn because each host
        # has certificates in their pillar
        if host_name == host_pillar.get_repl_source():
            raise HostHasActiveReplicsError(host_name, slaves=host)

    return hostutil.delete_host(
        MY_CLUSTER_TYPE,
        cluster,
        host_name,
        args={
            'zk_hosts': cluster['value']['data']['pgsync']['zk_hosts'],
        },
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
def list_postgresql_hosts(cluster, **_):
    """
    Returns list of PostgreSQL hosts.
    """
    hosts = get_host_objects(cluster, extra_formatter=postgres_host_extra_formatter)
    cid = str(cluster['cid'])

    fqdns = [host['name'] for host in hosts]
    healthinfo = MDBH.health_by_fqdns(fqdns)

    for host in hosts:
        host.update(diagnose_host(cid, host['name'], healthinfo))
    return {'hosts': hosts}


def _repl_source_for_host(host):
    return PostgresqlHostPillar(metadb.get_fqdn_pillar(fqdn=host)).get_repl_source()


_HOST_ROLE_FROM_SERVICE_ROLE = {
    ServiceRole.master: HostRole.master,
    ServiceRole.replica: HostRole.replica,
    ServiceRole.unknown: HostRole.unknown,
}


def host_role_from_service_role(role: ServiceRole) -> HostRole:
    """
    Converts host role from service role.
    """
    return _HOST_ROLE_FROM_SERVICE_ROLE.get(role, HostRole.unknown)


_HOST_REPLICA_TYPE_FROM_SERVICE_ROLE = {
    ServiceReplicaType.unknown: HostReplicaType.unknown,
    ServiceReplicaType.a_sync: HostReplicaType.a_sync,
    ServiceReplicaType.sync: HostReplicaType.sync,
}


def replica_type_from_service_to_host(replica_type: ServiceReplicaType) -> HostReplicaType:
    """
    Converts host replica type from service replica type.
    """
    return _HOST_REPLICA_TYPE_FROM_SERVICE_ROLE.get(replica_type, HostReplicaType.unknown)


_SERVICE_TYPE_FROM_MDBH = {
    'pg_replication': ServiceType.postgres,
    'pgbouncer': ServiceType.pooler,
}


def service_type_from_mdbh(name: str) -> ServiceType:
    """
    Converts service type from mdbh.
    """
    return _SERVICE_TYPE_FROM_MDBH.get(name, ServiceType.unspecified)


def diagnose_host(cid: str, hostname: str, healthinfo: HealthInfo) -> dict:
    """
    Return host health info.
    """

    services = []
    role = HostRole.unknown
    replica_type = HostReplicaType.unknown
    system = {}

    hoststatus = HostStatus.unknown
    hosthealth = healthinfo.host(hostname, cid)
    if hosthealth is not None:
        hoststatus = hosthealth.status()
        for shealth in hosthealth.services():
            stype = service_type_from_mdbh(shealth.name)

            if stype == ServiceType.postgres:
                role = host_role_from_service_role(shealth.role)
                replica_type = replica_type_from_service_to_host(shealth.replicaType)

            services.append(
                {
                    'type': stype,
                    'health': shealth.status,
                }
            )

        system = hosthealth.system_dict()

    hostdict = {
        'name': hostname,
        'role': role,
        'replica_type': replica_type,
        'health': hoststatus,
        'services': services,
    }
    if system:
        hostdict['system'] = system

    return hostdict
