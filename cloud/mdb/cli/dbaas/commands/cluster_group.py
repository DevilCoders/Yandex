from collections import OrderedDict
from datetime import datetime, timezone

from google.protobuf.wrappers_pb2 import Int64Value
from pkg_resources import parse_version

from click import argument, ClickException, group, option, pass_context
from cloud.mdb.cli.common.formatting import (
    format_bytes,
    print_response,
)
from cloud.mdb.cli.common.parameters import (
    BytesParamType,
    DateTimeParamType,
    FieldsParamType,
    JsonParamType,
    ListParamType,
)
from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.common.utils import ensure_no_unsupported_options
from cloud.mdb.cli.dbaas.internal.config import config_option
from cloud.mdb.cli.dbaas.internal.db import db_query
from cloud.mdb.cli.dbaas.internal.dbm.porto_container import get_porto_containers
from cloud.mdb.cli.dbaas.internal.dist import dist_find
from cloud.mdb.cli.dbaas.internal.grpc import grpc_request, grpc_service
from cloud.mdb.cli.dbaas.internal.intapi import (
    ClusterType,
    perform_operation_grpc,
    perform_operation_rest,
    rest_request,
)
from cloud.mdb.cli.dbaas.internal.metadb.cluster import (
    cluster_lock,
    get_cluster,
    get_cluster_stats,
    get_cluster_type,
    get_cluster_type_by_id,
    get_clusters,
    update_cluster,
)
from cloud.mdb.cli.dbaas.internal.metadb.common import to_cluster_type
from cloud.mdb.cli.dbaas.internal.metadb.exceptions import ClusterNotFound
from cloud.mdb.cli.dbaas.internal.metadb.host import get_hosts, delete_hosts
from cloud.mdb.cli.dbaas.internal.metadb.pillar import delete_pillar_key, update_pillar
from cloud.mdb.cli.dbaas.internal.metadb.shard import get_shards
from cloud.mdb.cli.dbaas.internal.metadb.subcluster import get_subcluster, get_subclusters, delete_subcluster
from cloud.mdb.cli.dbaas.internal.metadb.task import create_task, ParallelTaskExecuter, update_task, wait_task
from cloud.mdb.cli.dbaas.internal.utils import (
    cluster_status_options,
    DELETED_CLUSTER_STATUSES,
    format_hosts,
    format_references,
    ClusterStatus,
)
from yandex.cloud.priv.mdb.elasticsearch.v1 import (
    cluster_pb2 as es_cluster_pb2,
    cluster_service_pb2 as es_cluster_service_pb2,
    cluster_service_pb2_grpc as es_cluster_service_pb2_grpc,
    user_pb2 as es_user_pb2,
)
from yandex.cloud.priv.mdb.kafka.v1 import (
    cluster_pb2 as kafka_cluster_pb2,
    cluster_service_pb2 as kafka_cluster_service_pb2,
    cluster_service_pb2_grpc as kafka_cluster_service_pb2_grpc,
)


FIELD_FORMATTERS = {
    'type': to_cluster_type,
    'memory_size': format_bytes,
    'disk_size': format_bytes,
    'memory_cpu_ration': format_bytes,
}


class ClusterFields(FieldsParamType):
    name = 'cluster_fields'

    def __init__(self):
        super().__init__(
            [
                'id',
                'name',
                'type',
                'version',
                'env',
                'created_at',
                'status',
                'revision',
                'folder_id',
                'cloud_id',
                'network_id',
                'hosts',
                'cpu_cores',
                'memory_size',
                'disk_size',
            ]
        )


class ClusterStatsFields(FieldsParamType):
    name = 'cluster_stats_fields'

    def __init__(self):
        super().__init__(
            [
                'type',
                'version',
                'clusters',
                'hosts',
                'cpu_cores',
                'memory_size',
                'memory_cpu_ration',
                'disk_size',
            ]
        )


@group('cluster')
def cluster_group():
    """Cluster management commands."""
    pass


@cluster_group.command('get')
@argument('untyped_id', metavar='ID')
@option('--api', 'api_mode', is_flag=True, help='Perform request through REST API rather than DB query.')
@pass_context
def get_command(ctx, api_mode, untyped_id):
    """Get cluster.

    For getting cluster by related object, ID argument accepts subcluster ID, shard ID, hostname and
    task ID in addition to cluster ID."""
    try:
        cluster = get_cluster(ctx, cluster_id=untyped_id)
    except ClusterNotFound:
        cluster = get_cluster(ctx, untyped_id=untyped_id)

    command_impl = _get_cluster_api if api_mode else _get_cluster_db
    command_impl(ctx, cluster)


def _get_cluster_api(ctx, cluster):
    response = rest_request(ctx, 'GET', get_cluster_type(cluster), f'clusters/{cluster["id"]}')
    print_response(ctx, response)


def _get_cluster_db(ctx, cluster):
    subclusters = get_subclusters(ctx, cluster_id=cluster['id'])
    hosts = get_hosts(ctx, cluster_id=cluster['id'])

    result = OrderedDict((key, value) for key, value in cluster.items() if key not in 'pillar')
    result['subclusters'] = _format_subclusters(subclusters, hosts)
    result['settings'] = _format_settings(cluster, subclusters)
    result['references'] = _format_references(ctx, cluster, hosts)

    print_response(ctx, result)


def _format_references(ctx, cluster, hosts):
    macros = {
        'folder_id': cluster['folder_id'],
        'cluster_type': get_cluster_type(cluster),
        'cluster_id': cluster['id'],
    }
    if hosts:
        macros['host'] = (hosts[0]['public_fqdn'],)

    return format_references(ctx, 'cluster_references', macros)


def _format_settings(cluster, subclusters):
    settings = {}
    for subcluster in subclusters:
        if 'clickhouse_cluster' in subcluster['roles']:
            pillar = subcluster['pillar']
            settings['cloud_storage'] = bool(pillar['data'].get('cloud_storage', {}).get('enabled'))
            settings['embedded_keeper'] = bool(pillar['data']['clickhouse'].get('embedded_keeper'))
            settings['sql_user_management'] = bool(pillar['data']['clickhouse'].get('sql_user_management'))
            settings['sql_database_management'] = bool(pillar['data']['clickhouse'].get('sql_database_management'))

    return settings


def _format_subclusters(subclusters, hosts):
    result = []

    for sc in subclusters:
        sc_hosts = [h for h in hosts if h['subcluster_id'] == sc['id']]
        if len(sc_hosts) > 0:
            host = sc_hosts[0]
        else:
            host = {'flavor': None, 'disk_type': None, 'disk_size': 0}
        result.append(
            OrderedDict(
                (
                    ('id', sc['id']),
                    ('name', sc['name']),
                    ('roles', sc['roles']),
                    ('zones', ','.join(set(h['zone'] for h in sc_hosts))),
                    ('flavor', host['flavor']),
                    ('disk_type', host['disk_type']),
                    ('disk_size', format_bytes(host['disk_size'])),
                    ('hosts', format_hosts(sc_hosts)),
                )
            )
        )

    return result


@cluster_group.command('list')
@argument('untyped_ids', metavar='IDS', required=False, type=ListParamType())
@option('--api', 'api_mode', is_flag=True, help='Perform request through REST API rather than DB query.')
@option(
    '--name',
    help='Filter objects to output by cluster name. The value can be a pattern to match with'
    ' the syntax of LIKE clause patterns.',
)
@option(
    '-f',
    '--folder',
    '--folders',
    'folder_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several folders. Multiple values can be specified through a comma.',
)
@option('--cloud', 'cloud_id', help='Filter clusters to output by cloud.')
@option(
    '-c',
    '--cluster',
    '--clusters',
    'cluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several cluster IDs. Multiple values can be specified through a comma.',
)
@option(
    '--xcluster',
    '--exclude-cluster',
    '--exclude-clusters',
    'exclude_cluster_ids',
    type=ListParamType(),
    help='Filter objects to not output by one or several cluster IDs. Multiple values can be specified through a comma.',
)
@option(
    '-t',
    '--type',
    '--cluster-type',
    '--cluster-types',
    'cluster_types',
    type=ListParamType(ClusterType()),
    help='Filter objects to output by one or several cluster types. Multiple values can be specified through a comma.',
)
@option('-e', '--env', '--cluster-env', 'env', help='Filter objects to output by cluster environment.')
@cluster_status_options(with_short_aliases=True)
@option(
    '--sc',
    '--subcluster',
    '--subclusters',
    'subcluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several subcluster IDs. Multiple values can be specified through a comma.',
)
@option(
    '-H',
    '--host',
    '--hosts',
    'hostnames',
    type=ListParamType(),
    help='Filter objects to output by one or several host names. Multiple values can be specified through a comma.',
)
@option(
    '--vtype-id',
    '--vtype-ids',
    'vtype_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several vtype IDs. Multiple values can be specified through a comma.',
)
@option(
    '--task',
    '--tasks',
    'task_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several tasks. Multiple values can be specified through a comma.',
)
@option(
    '--dom0',
    'dom0_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several dom0 servers. Multiple values can be specified through a comma.',
)
@option('--version', help='Filter objects to output by cluster version.')
@option(
    '--pillar',
    '--pillar-match',
    'pillar_filter',
    metavar='JSONPATH_EXPRESSION',
    help='Filter objects by matching pillar against the specified jsonpath expression. The matching is performed'
    ' using jsonb_path_match PostgreSQL function.',
)
@option(
    '--subcluster-pillar',
    '--subcluster-pillar-match',
    'subcluster_pillar_filter',
    metavar='JSONPATH_EXPRESSION',
    help='Filter objects by matching any subcluster pillar against the specified jsonpath expression. The matching is '
    'performed using jsonb_path_match PostgreSQL function.',
)
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option(
    '--fields',
    type=ClusterFields(),
    default='id,name,type,env,created_at,status,version,hosts',
    help='Fields to output. The value "all" equals to all available fields.'
    ' Default is "id,name,type,env,created_at,status,version,hosts".',
)
@option('--order-by', type=str, default='created_at DESC')
@option('-q', '--quiet', is_flag=True, help='Output only cluster IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_command(ctx, api_mode, **kwargs):
    """List clusters.

    For getting clusters by related objects, IDs argument accepts subcluster IDs, shard IDs and hostnames
    in addition to cluster IDs. NOTE: applicable for DB mode only."""
    command_impl = _list_clusters_api if api_mode else _list_clusters_db
    command_impl(ctx, **kwargs)


def _list_clusters_api(ctx, folder_ids, cluster_types, quiet, separator, limit, **kwargs):
    ensure_no_unsupported_options(
        ctx,
        kwargs,
        {
            'untyped_ids': 'IDS',
            'name': '--name',
            'env': '--env',
            'dom0_ids': '--dom0',
            'cluster_ids': '--clusters',
            'subcluster_ids': '--subclusters',
            'hostnames': '--hosts',
            'cloud_id': '--cloud',
            'statuses': '--statuses',
            'exclude_statuses': '--exclude-statuses',
            'exclude_cluster_ids': '--exclude-clusters',
            'task_ids': '--tasks',
            'version': '--version',
            'pillar_filter': '--pillar-filter',
        },
        '{0} option is not supported in api mode.',
    )

    if not cluster_types:
        ctx.fail('--type option is required in api mode.')

    if len(cluster_types) != 1:
        ctx.fail('Multiple values for --type option cannot be specified in api mode.')
    cluster_type = cluster_types[0]

    if folder_ids:
        folder_id = folder_ids[0]
    else:
        folder_id = config_option(ctx, 'compute', 'folder')

    response = rest_request(
        ctx,
        'GET',
        cluster_type,
        'clusters',
        params={
            'folderId': folder_id,
        },
    )
    print_response(
        ctx,
        response['clusters'],
        default_format='table',
        fields=['id', 'name', 'environment', 'createdAt', 'status', 'health', 'folderId'],
        quiet=quiet,
        separator=separator,
        limit=limit,
    )


def _list_clusters_db(
    ctx,
    untyped_ids,
    cluster_ids,
    subcluster_ids,
    hostnames,
    dom0_ids,
    task_ids,
    cluster_statuses,
    exclude_cluster_statuses,
    limit,
    fields,
    quiet,
    separator,
    **kwargs,
):
    if dom0_ids and hostnames:
        ctx.fail(ctx, 'Options --dom0 and --hostnames cannot be used together.')

    if dom0_ids:
        hostnames = [c['fqdn'] for c in get_porto_containers(ctx, dom0_ids=dom0_ids)]

    if exclude_cluster_statuses is None and not any(
        (cluster_statuses, untyped_ids, cluster_ids, subcluster_ids, hostnames, task_ids)
    ):
        exclude_cluster_statuses = DELETED_CLUSTER_STATUSES

    clusters = get_clusters(
        ctx,
        untyped_ids=untyped_ids,
        cluster_ids=cluster_ids,
        subcluster_ids=subcluster_ids,
        hostnames=hostnames,
        task_ids=task_ids,
        statuses=cluster_statuses,
        exclude_statuses=exclude_cluster_statuses,
        with_stats=True,
        limit=limit,
        **kwargs,
    )
    print_response(
        ctx,
        clusters,
        default_format='table',
        field_formatters=FIELD_FORMATTERS,
        fields=fields,
        ignored_fields=['pillar'],
        quiet=quiet,
        separator=separator,
    )


@cluster_group.command('stats')
@option(
    '-t',
    '--type',
    '--cluster-type',
    'cluster_type',
    type=ClusterType(),
    help='Count clusters with the specified type only.',
)
@option('-r', '--role', help='Count hosts with the specified subcluster role only.')
@option('--zone', '--geo', 'zone', help='Count hosts with the specified availability zone only.')
@option(
    '--fields',
    type=ClusterStatsFields(),
    default='type,version,clusters,hosts,cpu_cores,memory_size,disk_size',
    help='Fields to output. The value "all" equals to all available fields.'
    ' Default is "type,version,clusters,hosts,cpu_cores,memory_size,disk_size".',
)
@pass_context
def get_cluster_stats_command(ctx, fields, **kwargs):
    """Ger statistics on clusters."""
    stats = get_cluster_stats(ctx, **kwargs)
    print_response(ctx, stats, default_format='table', field_formatters=FIELD_FORMATTERS, fields=fields)


@cluster_group.command('create')
@argument('cluster_type', metavar='TYPE', type=ClusterType())
@argument('name')
@option('-f', '--folder', 'folder_id')
@option('-e', '--environment', default='PRESTABLE')
@option('--network')
@option('-g', '--zones', '--zone', '--geo', 'zones')
@option('--flavor', '--resource-preset', 'flavor')
@option('--disk-type')
@option('--disk-size', type=BytesParamType(), default='10G')
@option('--shard-name')
@option('--host-count', default=1)
@option('--version', help="DBMS version.")
@option('--enable-cloud-storage', '--cloud-storage', 'cloud_storage', is_flag=True, help='Enable Cloud Storage.')
@option(
    '--enable-sql-user-management',
    '--sql-user-management',
    'sql_user_management',
    is_flag=True,
    help='Enable user management through SQL.',
)
@option(
    '--enable-sql-database-management',
    '--sql-database-management',
    'sql_database_management',
    is_flag=True,
    help='Enable database management through SQL.',
)
@option('--enable-embedded-keeper', '--embedded-keeper', 'embedded_keeper', is_flag=True, help='Enable Cloud Storage.')
@option('--service-account', '--service-account-id', 'service_account_id')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def create_command(
    ctx,
    cluster_type,
    name,
    folder_id,
    environment,
    network,
    zones,
    flavor,
    disk_type,
    disk_size,
    shard_name,
    host_count,
    version,
    cloud_storage,
    sql_user_management,
    sql_database_management,
    embedded_keeper,
    service_account_id,
    hidden,
):
    """Create cluster."""
    if not folder_id:
        folder_id = config_option(ctx, 'compute', 'folder')

    if not network:
        network = config_option(ctx, 'compute', 'network', None)

    if not zones:
        zones = config_option(ctx, 'compute', 'zones')

    zone_ids = [zone_id.strip() for zone_id in zones.split(',')]
    if not zone_ids:
        ctx.fail('Availability zones are not specified.')

    if not flavor:
        flavor = config_option(ctx, 'intapi', 'resource_preset')

    if not disk_type:
        disk_type = config_option(ctx, 'intapi', 'disk_type')

    if cloud_storage and cluster_type != 'clickhouse':
        raise ClickException('--cloud-storage option is applicable for ClickHouse clusters only.')

    if sql_user_management and cluster_type != 'clickhouse':
        raise ClickException('--sql-user-management option is applicable for ClickHouse clusters only.')

    if sql_database_management and cluster_type != 'clickhouse':
        raise ClickException('--sql-database-management option is applicable for ClickHouse clusters only.')

    if embedded_keeper and cluster_type != 'clickhouse':
        raise ClickException('--embedded-keeper option is applicable for ClickHouse clusters only.')

    if service_account_id and cluster_type != 'clickhouse':
        raise ClickException('--service-account option is applicable for ClickHouse clusters only.')

    if shard_name and cluster_type != 'clickhouse':
        raise ClickException('--shard-name option is applicable for ClickHouse clusters only.')

    if cluster_type == 'elasticsearch':
        create_function = _create_elasticsearch_cluster
    elif cluster_type == 'kafka':
        create_function = _create_kafka_cluster
    else:
        create_function = _create_cluster_rest

    create_function(
        ctx,
        cluster_type=cluster_type,
        name=name,
        folder_id=folder_id,
        environment=environment,
        network=network,
        zone_ids=zone_ids,
        flavor=flavor,
        disk_type=disk_type,
        disk_size=disk_size,
        host_count=host_count,
        version=version,
        shard_name=shard_name,
        cloud_storage=cloud_storage,
        sql_user_management=sql_user_management,
        sql_database_management=sql_database_management,
        embedded_keeper=embedded_keeper,
        service_account_id=service_account_id,
        hidden=hidden,
    )


def _create_cluster_rest(
    ctx,
    *,
    cluster_type,
    name,
    folder_id,
    environment,
    network,
    zone_ids,
    flavor,
    disk_type,
    disk_size,
    shard_name,
    host_count,
    version,
    cloud_storage,
    sql_user_management,
    sql_database_management,
    embedded_keeper,
    service_account_id,
    hidden,
    **_kwargs,
):
    resources = {
        'resourcePresetId': flavor,
        'diskSize': disk_size,
        'diskTypeId': disk_type,
    }

    if cluster_type == 'postgresql':
        data = {
            'configSpec': {
                'version': version or '10',
                'poolerConfig': {
                    'poolingMode': 'TRANSACTION',
                },
                'resources': resources,
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[i % len(zone_ids)],
                }
                for i in range(host_count)
            ],
            'databaseSpecs': [
                {
                    'name': name,
                    'owner': 'dbuser',
                }
            ],
            'userSpecs': [
                {
                    'name': 'dbuser',
                    'password': '12345678',
                }
            ],
        }

    elif cluster_type == 'clickhouse':
        data = {
            'configSpec': {
                'clickhouse': {
                    'resources': resources,
                },
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[i % len(zone_ids)],
                    'type': 'CLICKHOUSE',
                }
                for i in range(host_count)
            ],
        }
        if version:
            data['configSpec']['version'] = version
        if sql_user_management:
            data['configSpec']['sqlUserManagement'] = True
            data['configSpec']['adminPassword'] = '12345678'
            data['userSpecs'] = []
        else:
            data['userSpecs'] = [
                {
                    'name': 'dbuser',
                    'password': '12345678',
                }
            ]
        if sql_database_management:
            data['configSpec']['sqlDatabaseManagement'] = True
            data['databaseSpecs'] = []
        else:
            data['databaseSpecs'] = [
                {
                    'name': name,
                }
            ]
        if cloud_storage:
            data['configSpec']['cloudStorage'] = {'enabled': True}
        if embedded_keeper:
            data['configSpec']['embeddedKeeper'] = True
        if service_account_id:
            data['serviceAccountId'] = service_account_id

    elif cluster_type == 'mongodb':
        if version:
            ctx.fail('--version option is not supported for MongoDB clusters.')

        data = {
            'configSpec': {
                'mongodbSpec_3_6': {
                    'mongod': {
                        'resources': resources,
                    },
                },
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[i % len(zone_ids)],
                }
                for i in range(host_count)
            ],
            'databaseSpecs': [
                {
                    'name': name,
                }
            ],
            'userSpecs': [
                {
                    'name': 'dbuser',
                    'password': '12345678',
                }
            ],
        }

    elif cluster_type == 'mysql':
        if not version:
            version = '8.0'

        data = {
            'configSpec': {
                'version': version,
                'resources': resources,
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[i % len(zone_ids)],
                }
                for i in range(host_count)
            ],
            'databaseSpecs': [
                {
                    'name': name,
                }
            ],
            'userSpecs': [
                {
                    'name': 'dbuser',
                    'password': '12345678',
                }
            ],
        }

    elif cluster_type == 'redis':
        if version:
            ctx.fail('--version option is not supported for Redis clusters.')

        data = {
            'configSpec': {
                'redisConfig_5_0': {
                    'password': '12345678',
                },
                'resources': resources,
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[i % len(zone_ids)],
                }
                for i in range(host_count)
            ],
        }

    else:
        raise ctx.fail('Invalid cluster type.')

    if network:
        data['networkId'] = network

    if shard_name:
        data['shardName'] = shard_name

    perform_operation_rest(
        ctx,
        'POST',
        cluster_type,
        'clusters',
        hidden=hidden,
        data={
            'folderId': folder_id,
            'name': name,
            'environment': environment,
            **data,
        },
    )


def _create_elasticsearch_cluster(
    ctx,
    *,
    name,
    folder_id,
    environment,
    network,
    zone_ids,
    flavor,
    disk_type,
    disk_size,
    host_count,
    version,
    hidden,
    **_kwargs,
):
    config_spec = es_cluster_service_pb2.ConfigSpec(
        elasticsearch_spec=es_cluster_service_pb2.ElasticsearchSpec(
            data_node=es_cluster_service_pb2.ElasticsearchSpec.DataNode(
                resources=es_cluster_pb2.Resources(
                    resource_preset_id=flavor,
                    disk_size=disk_size,
                    disk_type_id=disk_type,
                )
            )
        )
    )
    if version:
        config_spec.version = version

    host_specs = [
        es_cluster_service_pb2.HostSpec(
            zone_id=zone_ids[i % len(zone_ids)],
            type=es_cluster_pb2.Host.DATA_NODE,
        )
        for i in range(host_count)
    ]

    request = es_cluster_service_pb2.CreateClusterRequest(
        folder_id=folder_id,
        name=name,
        environment=es_cluster_pb2.Cluster.Environment.Value(environment),
        network_id=network,
        config_spec=config_spec,
        user_specs=[es_user_pb2.UserSpec(name='dbuser', password='12345678')],
        host_specs=host_specs,
    )

    perform_operation_grpc(ctx, _es_cluster_service(ctx).Create, request, hidden=hidden)


def _create_kafka_cluster(
    ctx,
    *,
    name,
    folder_id,
    environment,
    network,
    zone_ids,
    flavor,
    disk_type,
    disk_size,
    host_count,
    version,
    hidden,
    **_kwargs,
):
    if host_count:
        ctx.fail('--host-count option is not supported for Kafka clusters.')

    config_spec = kafka_cluster_pb2.ConfigSpec(
        kafka=kafka_cluster_pb2.ConfigSpec.Kafka(
            resources=kafka_cluster_pb2.Resources(
                resource_preset_id=flavor,
                disk_size=disk_size,
                disk_type_id=disk_type,
            )
        ),
        zone_id=[zone_ids[0]],
        brokers_count=Int64Value(value=1),
    )
    if version:
        config_spec.version = version

    request = kafka_cluster_service_pb2.CreateClusterRequest(
        folder_id=folder_id,
        name=name,
        environment=kafka_cluster_pb2.Cluster.Environment.Value(environment),
        network_id=network,
        config_spec=config_spec,
    )

    perform_operation_grpc(ctx, _kafka_cluster_service(ctx).Create, request, hidden=hidden)


@cluster_group.command('stop')
@argument('cluster_id', metavar='CLUSTER')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def stop_command(ctx, cluster_id, hidden):
    """Stop cluster."""
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(ctx, 'POST', cluster_type, f'clusters/{cluster_id}:stop', hidden=hidden)


@cluster_group.command('start')
@argument('cluster_id', metavar='CLUSTER')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def start_command(ctx, cluster_id, hidden):
    """Start cluster."""
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(ctx, 'POST', cluster_type, f'clusters/{cluster_id}:start', hidden=hidden)


@cluster_group.command('update-environment')
@argument('cluster_id', metavar='CLUSTER')
@argument('environment')
@option('--metadb-only', is_flag=True)
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def update_environment_command(ctx, cluster_id, environment, metadb_only, hidden, force):
    """Update cluster environment."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    task_id = None
    with cluster_lock(ctx, cluster_id):
        cluster_type = get_cluster_type_by_id(ctx, cluster_id)
        update_cluster(ctx, cluster_id, environment=environment)
        if not metadb_only:
            task_id = create_task(
                ctx,
                cluster_id=cluster_id,
                task_type=f'{cluster_type}_cluster_modify',
                operation_type=f'{cluster_type}_cluster_modify',
                hidden=hidden,
            )

    if task_id:
        wait_task(ctx, task_id)


@cluster_group.command('update-status')
@argument('cluster_id', metavar='CLUSTER')
@argument('status', type=ClusterStatus())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def update_status_command(ctx, cluster_id, status, force):
    """Update cluster status."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    with cluster_lock(ctx, cluster_id):
        update_cluster(ctx, cluster_id, status=status)


@cluster_group.command('upgrade')
@argument('cluster_ids', metavar='CLUSTERS', type=ListParamType())
@argument('version')
@option(
    '--from',
    'source_version',
    help='Source ClickHouse version. The upgrade will be skipped for clusters with a mismatched version.',
)
@option('--metadb-only', is_flag=True, help='Only update data in metadb.')
@option('--force', is_flag=True, help='Skip version check.')
@option('--hidden', is_flag=True, help='Mark underlying tasks as hidden.')
@option('--timeout', default='3 hours', help='Task timeout.')
@option(
    '--no-health',
    '--no-health-check',
    '--disable-health-check',
    'disable_health_check',
    is_flag=True,
    help='Omit all health checks during task execution.',
)
@option(
    '--fast-mode',
    '--fast',
    'fast_mode',
    is_flag=True,
    help='Deploy on hosts with maximal parallelism level. Applicable for ClickHouse clusters for now.',
)
@option('--task-args', type=JsonParamType(), help='Additional task arguments.')
@option('-p', '--parallel', type=int, default=8, help='Maximum number of tasks to run in parallel.')
@option('-k', '--keep-going', is_flag=True, help='Do not stop on the first failed task.')
@pass_context
def upgrade_command(
    ctx,
    cluster_ids,
    version,
    metadb_only,
    force,
    hidden,
    timeout,
    disable_health_check,
    fast_mode,
    source_version,
    task_args,
    parallel,
    keep_going,
):
    """Upgrade version of one or several clusters."""
    upgrade_traits = {
        'clickhouse': {
            'pkg': 'clickhouse-server',
            'pillar_type': 'subcluster',
            'subcluster_role': 'clickhouse_cluster',
            'get_version': lambda p: p['data']['clickhouse']['ch_version'],
            'set_version': lambda p, v: p['data']['clickhouse'].update({'ch_version': v}),
            'task_type': 'clickhouse_cluster_upgrade',
            'operation_type': 'clickhouse_cluster_modify',
        },
        'redis': {
            'pkg': 'redis-server',
            'pillar_type': 'cluster',
            'get_version': lambda p: p['data']['redis']['version'].get('pkg'),
            'set_version': lambda p, v: p['data']['redis']['version'].update({'pkg': v}),
        },
    }

    if not cluster_ids:
        ctx.fail('No clusters to upgrade are provided.')

    cluster_type = None
    for cluster_id in cluster_ids:
        if not cluster_type:
            cluster_type = get_cluster_type_by_id(ctx, cluster_id)
        else:
            if get_cluster_type_by_id(ctx, cluster_id) != cluster_type:
                ctx.fail('Clusters must have the same cluster type.')

    if cluster_type not in upgrade_traits:
        ctx.fail(f'The command is not supported for {cluster_type} clusters.')

    traits = upgrade_traits[cluster_type]

    if ('task_type' not in traits) and not metadb_only:
        ctx.fail(
            f'The command is not fully supported for {cluster_type} clusters.' f' The option --metadb-only is required.'
        )

    if not force:
        pkg = traits['pkg']
        dist_response = dist_find(pkg, 'mdb-bionic-secure', 'stable', version)
        if not dist_response:
            ctx.fail(f'{pkg} {version} was not found in the dist repository "mdb-bionic-secure" "stable".')

    def _upgrade(cluster_id):
        with cluster_lock(ctx, cluster_id) as txn:
            cluster = get_cluster(ctx, cluster_id)
            if traits['pillar_type'] == 'subcluster':
                subcluster = get_subcluster(ctx, cluster_id=cluster_id, role=traits['subcluster_role'])
                pillar = subcluster['pillar']
            else:
                subcluster = None
                pillar = cluster['pillar']

            current_version = traits['get_version'](pillar)
            if version == current_version:
                print(f'Cluster {cluster_id}: skipped as the current version is already {version}')
                txn.rollback()
                return None
            if source_version and current_version != source_version:
                print(
                    f'Cluster {cluster_id}: skipped as the actual version ({version}) mismatched'
                    f' expected version ({source_version})'
                )
                txn.rollback()
                return None

            traits['set_version'](pillar, version)
            if traits['pillar_type'] == 'subcluster':
                update_pillar(ctx, subcluster_id=subcluster['id'], value=pillar)
            else:
                update_pillar(ctx, cluster_id=cluster_id, value=pillar)

            if metadb_only:
                print(
                    f'Cluster {cluster_id}: updated {cluster_type} version'
                    f' from {current_version} to {version} in metadb'
                )
                return None

            args = task_args or {}
            args['restart'] = True
            args['fast_mode'] = fast_mode
            args['disable_health_check'] = disable_health_check
            args['version_from'] = current_version
            args['version_to'] = version

            task_id = create_task(
                ctx,
                cluster_id=cluster_id,
                task_type=traits['task_type'],
                operation_type=traits['operation_type'],
                task_args=args,
                hidden=hidden,
                timeout=timeout,
                revision=cluster['revision'],
            )

            if len(cluster_ids) > 1:
                print(
                    f'Cluster {cluster_id}: scheduled task to update {cluster_type} version'
                    f' from {current_version} to {version}'
                )

            return task_id

    executer = ParallelTaskExecuter(ctx, parallel, keep_going=keep_going)
    for cluster_id in cluster_ids:
        executer.submit(_upgrade, cluster_id)

    completed, failed = executer.run()

    if failed and len(cluster_ids) > 1:
        ctx.fail(f'Failed tasks: {", ".join(failed)}')


@cluster_group.command('restart')
@argument('cluster_ids', metavar='CLUSTERS', type=ListParamType())
@option('--hidden', is_flag=True, help='Mark underlying tasks as hidden.')
@option(
    '--no-health',
    '--no-health-check',
    '--disable-health-check',
    'disable_health_check',
    is_flag=True,
    help='Omit all health checks during task execution.',
)
@option(
    '--fast-mode',
    '--fast',
    'fast_mode',
    is_flag=True,
    help='Deploy on hosts with maximal parallelism level. Applicable for ClickHouse clusters for now.',
)
@option('-p', '--parallel', type=int, default=8, help='Maximum number of tasks to run in parallel.')
@option('-k', '--keep-going', is_flag=True, help='Do not stop on the first failed task.')
@pass_context
def restart_command(ctx, cluster_ids, hidden, parallel, keep_going, disable_health_check, fast_mode):
    """Restart one or several clusters."""

    def _restart(cluster_id):
        with cluster_lock(ctx, cluster_id):
            cluster_type = get_cluster(ctx, cluster_id)['type']
            task_id = create_task(
                ctx,
                cluster_id=cluster_id,
                task_type=f'{cluster_type}_modify',
                operation_type=f'{cluster_type}_modify',
                task_args={
                    'restart': True,
                    'fast_mode': fast_mode,
                    'disable_health_check': disable_health_check,
                },
                hidden=hidden,
            )
            print(f'Cluster {cluster_id}: scheduled task to restart cluster')
            return task_id

    executer = ParallelTaskExecuter(ctx, parallel, keep_going=keep_going)
    for cluster_id in cluster_ids:
        executer.submit(_restart, cluster_id)

    completed, failed = executer.run()

    if failed and len(cluster_ids) > 1:
        ctx.fail(f'Failed tasks: {", ".join(failed)}')


@cluster_group.command('maintenance')
@argument('cluster_ids', metavar='CLUSTERS', type=ListParamType())
@option(
    '--update-tls',
    '--update-tls-certs',
    'update_tls_certs',
    is_flag=True,
    help='Update TLS certificates during maintenance.',
)
@option('--timeout', default='3 hours', help='Task timeout.')
@option('--delay', default='0', help='Delay task for the specified amount of time (e.g. "7 days").')
@option(
    '--delay-until', type=DateTimeParamType(), help='Delay task until the specified time (e.g. "2031-03-01 20:30:00").'
)
@option(
    '--no-health',
    '--no-health-check',
    '--disable-health-check',
    'disable_health_check',
    is_flag=True,
    help='Omit all health checks during task execution.',
)
@option(
    '--fast-mode',
    '--fast',
    'fast_mode',
    is_flag=True,
    help='Deploy on hosts with maximal parallelism level. Applicable for ClickHouse clusters for now.',
)
@option('--task-args', type=JsonParamType(), help='Additional task arguments.')
@option('--hidden', is_flag=True, help='Mark underlying tasks as hidden.')
@option('-p', '--parallel', type=int, default=8, help='Maximum number of tasks to run in parallel.')
@option('-k', '--keep-going', is_flag=True, help='Do not stop on the first failed task.')
@pass_context
def maintenance_command(
    ctx,
    cluster_ids,
    update_tls_certs,
    timeout,
    delay,
    delay_until,
    disable_health_check,
    fast_mode,
    task_args,
    hidden,
    parallel,
    keep_going,
):
    """Perform maintenance of one or several clusters."""

    if delay_until:
        delay = str(int((delay_until - datetime.now(timezone.utc)).total_seconds()))

    def _maintenance(cluster):
        cluster_id = cluster['id']
        cluster_type = cluster['type']
        with cluster_lock(ctx, cluster_id):
            args = task_args or {}
            args['restart'] = True
            if update_tls_certs:
                args['update_tls'] = True
                args['force_tls_certs'] = True
            if disable_health_check:
                args['disable_health_check'] = True
            if fast_mode:
                args['fast_mode'] = True
            if cluster_type == 'clickhouse_cluster':
                args['restart_zk'] = True

            task_id = create_task(
                ctx,
                cluster_id=cluster_id,
                task_type=f'{cluster_type}_maintenance',
                operation_type=f'{cluster_type}_modify',
                task_args=args,
                timeout=timeout,
                delay=delay,
                hidden=hidden,
            )
            print(f'Cluster {cluster_id}: scheduled cluster maintenance task')
            return task_id

    executer = ParallelTaskExecuter(ctx, parallel, keep_going=keep_going)
    for cluster in _get_clusters(ctx, cluster_ids):
        executer.submit(_maintenance, cluster)

    completed, failed = executer.run()

    if failed and len(cluster_ids) > 1:
        ctx.fail(f'Failed tasks: {", ".join(failed)}')


@cluster_group.command('update-tls-certs')
@argument('cluster_ids', metavar='CLUSTERS', type=ListParamType())
@option('--hidden', is_flag=True, help='Mark underlying tasks as hidden.')
@option('-p', '--parallel', type=int, default=8, help='Maximum number of tasks to run in parallel.')
@option('-k', '--keep-going', is_flag=True, help='Do not stop on the first failed task.')
@pass_context
def update_tls_certs_command(ctx, cluster_ids, hidden, parallel, keep_going):
    """Update TLS certificates for one or several clusters."""

    def _update_tls_certs(cluster_id):
        with cluster_lock(ctx, cluster_id):
            for host in get_hosts(ctx, cluster_id=cluster_id):
                delete_pillar_key(ctx, host=host['fqdn'], key='cert.key')
                delete_pillar_key(ctx, host=host['fqdn'], key='cert.crt')

            cluster_type = get_cluster(ctx, cluster_id)['type']
            task_id = create_task(
                ctx,
                cluster_id=cluster_id,
                task_type=f'{cluster_type}_update_tls_certs',
                operation_type=f'{cluster_type}_update_tls_certs',
                task_args={
                    'force_tls_certs': True,
                },
                hidden=hidden,
            )
            print(f'Cluster {cluster_id}: scheduled task to update TLS certificates')
            return task_id

    executer = ParallelTaskExecuter(ctx, parallel, keep_going=keep_going)
    for cluster_id in cluster_ids:
        executer.submit(_update_tls_certs, cluster_id)

    completed, failed = executer.run()

    if failed and len(cluster_ids) > 1:
        ctx.fail(f'Failed tasks: {", ".join(failed)}')


@cluster_group.command('backup')
@argument('cluster_id', metavar='CLUSTER')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def backup_command(ctx, cluster_id, hidden):
    """Create cluster backup."""
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(ctx, 'POST', cluster_type, f'clusters/{cluster_id}:backup', hidden=hidden)


@cluster_group.command('restore')
@argument('type', type=ClusterType())
@argument('backup')
@argument('name')
@option('-f', '--folder')
@option('-e', '--environment', default='PRESTABLE')
@option('--network')
@option('-g', '--zones', '--zone', '--geo', 'zones')
@option('--flavor', '--resource-preset', 'flavor')
@option('--disk-type')
@option('--disk-size', type=BytesParamType(), default='10G')
@option('--shard-name')
@option('--host-count', default=1)
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def restore_command(
    ctx,
    type,
    backup,
    name,
    folder,
    environment,
    network,
    zones,
    flavor,
    disk_type,
    disk_size,
    shard_name,
    host_count,
    hidden,
):
    """Restore cluster from backup."""
    if not folder:
        folder = config_option(ctx, 'compute', 'folder')

    if not network:
        network = config_option(ctx, 'compute', 'network', None)

    if not zones:
        zones = config_option(ctx, 'compute', 'zones')
    zone_ids = [zone_id.strip() for zone_id in zones.split(',')]

    if not flavor:
        flavor = config_option(ctx, 'intapi', 'resource_preset')

    if not disk_type:
        disk_type = config_option(ctx, 'intapi', 'disk_type')

    assert len(zone_ids) >= host_count

    resources = {
        'resourcePresetId': flavor,
        'diskSize': disk_size,
        'diskTypeId': disk_type,
    }

    if type == 'postgresql':
        data = {
            'configSpec': {
                'resources': resources,
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[i],
                }
                for i in range(host_count)
            ],
        }
    elif type == 'clickhouse':
        data = {
            'configSpec': {
                'clickhouse': {
                    'resources': resources,
                },
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[i],
                    'type': 'CLICKHOUSE',
                }
                for i in range(host_count)
            ],
        }
    elif type == 'mongodb':
        data = {
            'configSpec': {
                'mongodbSpec_3_6': {
                    'mongod': {
                        'resources': resources,
                    },
                },
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[i],
                }
                for i in range(host_count)
            ],
        }
    elif type == 'mysql':
        data = {
            'configSpec': {
                'mysqlSpec_5_7': {
                    'mysql': {
                        'resources': resources,
                    },
                },
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[i],
                }
                for i in range(host_count)
            ],
        }
    elif type == 'redis':
        data = {
            'configSpec': {
                'resources': resources,
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[i],
                }
                for i in range(host_count)
            ],
        }
    else:
        assert False, 'Invalid cluster type.'

    if network:
        data['networkId'] = network

    if shard_name:
        data['shardName'] = shard_name

    perform_operation_rest(
        ctx,
        'POST',
        type,
        'clusters:restore',
        hidden=hidden,
        data={
            'folderId': folder,
            'backupId': backup,
            'name': name,
            'environment': environment,
            **data,
        },
    )


@cluster_group.command('enable-sharding')
@argument('cluster_id', metavar='CLUSTER')
@option('-g', '--zones', '--zone', '--geo', 'zones')
@option('--flavor', '--resource-preset', 'flavor')
@option('--disk-type')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def enable_sharding_command(ctx, cluster_id, zones, flavor, disk_type, hidden):
    """Enable sharding (applicable for MongoDB clusters only)."""
    if not zones:
        zones = config_option(ctx, 'compute', 'zones')
    zone_ids = [zone_id.strip() for zone_id in zones.split(',')]

    if not flavor:
        flavor = config_option(ctx, 'intapi', 'resource_preset')

    if not disk_type:
        disk_type = config_option(ctx, 'intapi', 'disk_type')

    assert len(zone_ids) >= 3

    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    if cluster_type != 'mongodb':
        ctx.fail(f'The command is not supported for {cluster_type} clusters.')

    perform_operation_rest(
        ctx,
        'POST',
        cluster_type,
        f'clusters/{cluster_id}:enableSharding',
        hidden=hidden,
        data={
            'mongos': {
                'resources': {
                    'resourcePresetId': flavor,
                    'diskSize': 10737418240,
                    'diskTypeId': disk_type,
                },
            },
            'mongocfg': {
                'resources': {
                    'resourcePresetId': flavor,
                    'diskSize': 10737418240,
                    'diskTypeId': disk_type,
                },
            },
            'hostSpecs': [
                {
                    'zoneId': zone_ids[0],
                    'type': 'MONGOS',
                },
                {
                    'zoneId': zone_ids[1],
                    'type': 'MONGOS',
                },
                {
                    'zoneId': zone_ids[2],
                    'type': 'MONGOS',
                },
                {
                    'zoneId': zone_ids[0],
                    'type': 'MONGOCFG',
                },
                {
                    'zoneId': zone_ids[1],
                    'type': 'MONGOCFG',
                },
                {
                    'zoneId': zone_ids[2],
                    'type': 'MONGOCFG',
                },
            ],
        },
    )


@cluster_group.command('enable-cloud-storage')
@argument('cluster_id', metavar='CLUSTER')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def enable_cloud_storage_command(ctx, cluster_id, hidden):
    """Enable cloud storage (applicable for ClickHouse clusters only)."""
    with cluster_lock(ctx, cluster_id):
        cluster = get_cluster(ctx, cluster_id)
        cluster_type = get_cluster_type(cluster)
        if cluster_type != 'clickhouse':
            ctx.fail(f'The command is not supported for {cluster_type} clusters.')

        subcluster = get_subcluster(ctx, cluster_id=cluster_id, role='clickhouse_cluster')
        pillar = subcluster['pillar']

        cloud_storage = pillar['data'].get('cloud_storage', {})
        if cloud_storage.get('enabled'):
            raise ClickException(f'Cloud storage is already enabled for cluster {cluster_id}')

        if parse_version(pillar['data']['clickhouse']['ch_version']) < parse_version('21.7'):
            raise ClickException('ClickHouse version must be 21.7 or higher in order to enable cloud storage.')

        cloud_storage_bucket = 'cloud-storage-' + cluster_id
        cloud_storage['enabled'] = True
        cloud_storage['s3'] = {'bucket': cloud_storage_bucket}

        update_pillar(ctx, subcluster_id=subcluster['id'], value=pillar)

        task_id = create_task(
            ctx,
            cluster_id=cluster_id,
            task_type='clickhouse_cluster_modify',
            operation_type='clickhouse_cluster_modify',
            task_args={
                'enable_cloud_storage': True,
                's3_buckets': {'cloud_storage': cloud_storage_bucket},
            },
            hidden=hidden,
            revision=cluster['revision'],
        )

    wait_task(ctx, task_id)


@cluster_group.command('add-zookeeper')
@argument('cluster_id', metavar='CLUSTER')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def add_zookeeper_command(ctx, cluster_id, hidden):
    """Add ZooKeeper to the cluster (applicable for ClickHouse clusters only)."""
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    if cluster_type != 'clickhouse':
        ctx.fail(f'The command is not supported for {cluster_type} clusters.')

    perform_operation_rest(ctx, 'POST', cluster_type, f'clusters/{cluster_id}:addZookeeper', hidden=hidden)


@cluster_group.command('remove-zookeeper')
@argument('cluster_id', metavar='CLUSTER')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def remove_zookeeper_command(ctx, cluster_id, hidden, force):
    """Remove ZooKeeper from the cluster (applicable for ClickHouse clusters only).

    ZooKeeper can be removed only for clusters with ClickHouse Keeper or for non-HA clusters with single host per shard.
    """
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    with cluster_lock(ctx, cluster_id):
        cluster = get_cluster(ctx, cluster_id)
        cluster_type = get_cluster_type(cluster)
        if cluster_type != 'clickhouse':
            raise ClickException(f'The command is not supported for {cluster_type} clusters.')

        zk_subcluster = get_subcluster(ctx, cluster_id=cluster_id, role='zk')
        if not zk_subcluster:
            raise ClickException(f'Cluster {cluster_id} has no ZooKeeper.')

        ch_subcluster = get_subcluster(ctx, cluster_id=cluster_id, role='clickhouse_cluster')
        ch_pillar = ch_subcluster['pillar']
        embedded_keeper = bool(ch_pillar['data']['clickhouse'].get('embedded_keeper'))

        ch_hosts = get_hosts(ctx, subcluster_id=ch_subcluster['id'])
        ch_shards = get_shards(ctx, subcluster_id=ch_subcluster['id'])

        if not embedded_keeper and len(ch_hosts) > len(ch_shards):
            raise ClickException(
                'The command can be performed only for clusters with ClickHouse Keeper or'
                ' for non-HA clusters with single host per shard.'
            )

        revision = cluster['revision']

        zk_hosts = get_hosts(ctx, subcluster_id=zk_subcluster['id'])
        delete_hosts(ctx, cluster_id, zk_hosts, revision)
        delete_subcluster(ctx, cluster_id, zk_subcluster['id'], revision)
        task_id = create_task(
            ctx,
            cluster_id=cluster_id,
            task_type='clickhouse_remove_zookeeper',
            operation_type='clickhouse_remove_zookeeper',
            hidden=hidden,
            task_args={
                'delete_hosts': [
                    {
                        "fqdn": host['public_fqdn'],
                        "subcid": host['subcluster_id'],
                        "vtype": host['vtype'],
                        "vtype_id": host['vtype_id'],
                    }
                    for host in zk_hosts
                ]
            },
            revision=revision,
        )

    wait_task(ctx, task_id)


@cluster_group.command('delete')
@argument('cluster_ids', metavar='CLUSTERS', type=ListParamType())
@option('--hidden', is_flag=True, help='Mark underlying tasks as hidden.')
@option('-p', '--parallel', type=int, default=8, help='Maximum number of clusters to process in parallel.')
@option('-k', '--keep-going', is_flag=True, help='Do not stop on the first failed task.')
@pass_context
def delete_command(ctx, cluster_ids, hidden, parallel, keep_going):
    """Delete one or several clusters."""

    def _delete(cluster_id):
        cluster_type = get_cluster_type_by_id(ctx, cluster_id)
        if cluster_type == 'elasticsearch':
            return _es_delete(cluster_id)
        elif cluster_type == 'kafka':
            return _kafka_delete(cluster_id)
        else:
            return _delete_rest(cluster_id, cluster_type)

    def _delete_rest(cluster_id, cluster_type):
        response = rest_request(ctx, 'DELETE', cluster_type, f'clusters/{cluster_id}')

        if hidden:
            update_task(ctx, response['id'], hidden=hidden)

        if len(cluster_ids) > 1:
            print(f'Cluster {cluster_id}: deletion started')

        return response['id']

    def _es_delete(cluster_id):
        request = es_cluster_service_pb2.DeleteClusterRequest(cluster_id=cluster_id)

        response = grpc_request(ctx, _es_cluster_service(ctx).Delete, request)

        if hidden:
            update_task(ctx, response.id, hidden=hidden)

        return response.id

    def _kafka_delete(cluster_id):
        request = kafka_cluster_service_pb2.DeleteClusterRequest(cluster_id=cluster_id)

        response = grpc_request(ctx, _kafka_cluster_service(ctx).Delete, request)

        if hidden:
            update_task(ctx, response.id, hidden=hidden)

        return response.id

    executer = ParallelTaskExecuter(ctx, parallel, keep_going=keep_going)
    for cluster_id in cluster_ids:
        executer.submit(_delete, cluster_id)

    completed, failed = executer.run()

    if failed and len(cluster_ids) > 1:
        ctx.fail(f'Failed operations: {", ".join(failed)}')


@cluster_group.command('revert')
@argument('cluster_id', metavar='CLUSTER')
@argument('revision', metavar='REVISION', type=int)
@argument('comment', metavar='COMMENT')
@pass_context
def revert_command(ctx, cluster_id, revision, comment):
    """Revert cluster to the specified revision."""
    db_query(
        ctx,
        'metadb',
        """
             SELECT code.revert_cluster_to_rev(
                 i_cid    => %(cluster_id)s,
                 i_rev    => %(revision)s,
                 i_reason => %(comment)s);
             """,
        cluster_id=cluster_id,
        revision=revision,
        comment=comment,
    )


@cluster_group.command('get-cluster-type-config')
@argument('type', type=ClusterType())
@option('-t', '--config-type', default='config')
@option('-f', '--folder')
@pass_context
def get_cluster_type_config_command(ctx, type, config_type, folder):
    if not folder:
        folder = config_option(ctx, 'compute', 'folder')

    response = rest_request(
        ctx,
        'GET',
        type,
        f'/console/clusters:{config_type}',
        params={
            'folderId': folder,
        },
    )

    print_response(ctx, response)


def _get_clusters(ctx, cluster_ids):
    clusters = get_clusters(ctx, cluster_ids=cluster_ids)

    if not clusters:
        raise ClickException('No clusters found.')

    if len(cluster_ids) != len(clusters):
        not_found_ids = set(cluster_ids) - set(cluster['id'] for cluster in clusters)
        if len(not_found_ids) == 1:
            raise ClickException(f'Cluster "{not_found_ids.pop()}" not found.')
        else:
            raise ClickException(f'Multiple clusters were not found: {",".join(not_found_ids)}')

    return clusters


def _es_cluster_service(ctx):
    return grpc_service(ctx, 'intapi', es_cluster_service_pb2_grpc.ClusterServiceStub)


def _kafka_cluster_service(ctx):
    return grpc_service(ctx, 'intapi', kafka_cluster_service_pb2_grpc.ClusterServiceStub)
