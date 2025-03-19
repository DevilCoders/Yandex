from collections import OrderedDict

from click import argument, echo, group, option, pass_context
from cloud.mdb.cli.dbaas.internal import resourcemanager
from cloud.mdb.cli.common.formatting import format_bytes, print_response
from cloud.mdb.cli.common.parameters import BytesParamType, ListParamType
from cloud.mdb.cli.dbaas.internal.grpc import grpc_request, grpc_service
from cloud.mdb.cli.dbaas.internal.metadb.cloud import (
    disable_feature_on_cloud,
    enable_feature_on_cloud,
    get_cloud,
    get_cloud_features,
    get_clouds,
    update_cloud_quota,
)
from cloud.mdb.cli.dbaas.internal.rest import rest_request

from yandex.cloud.priv.mdb.v1.console import cluster_service_pb2, cluster_service_pb2_grpc

FIELD_FORMATTERS = {
    'memory_used': format_bytes,
    'memory_quota': format_bytes,
    'ssd_space_used': format_bytes,
    'ssd_space_quota': format_bytes,
    'hdd_space_used': format_bytes,
    'hdd_space_quota': format_bytes,
}


@group('cloud')
def cloud_group():
    """Cloud management commands."""
    pass


@cloud_group.command('get')
@argument('cloud_id', metavar='ID')
@pass_context
def get_command(ctx, cloud_id):
    """Get cloud."""
    cloud = get_cloud(ctx, cloud_id)

    result = OrderedDict((key, value) for key, value in cloud.items())
    try:
        result['iam_features'] = ','.join(sorted(_get_iam_features(ctx, cloud_id)))
    except Exception:
        # TODO: log error
        pass
    result['metadb_features'] = ','.join(sorted(get_cloud_features(ctx, cloud_id)))

    print_response(ctx, result, field_formatters=FIELD_FORMATTERS)


@cloud_group.command('list')
@option('-c', '--clusters', 'cluster_ids', type=ListParamType())
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only cloud IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_command(ctx, cluster_ids, limit, quiet, separator):
    """List clouds."""

    def _table_formatter(cloud):
        return OrderedDict(
            (
                ('id', cloud['id']),
                ('clusters', f'{cloud["clusters_used"]} / {cloud["clusters_quota"]}'),
                ('CPU used', f'{cloud["cpu_used"]} / {cloud["cpu_quota"]}'),
                ('memory used', f'{cloud["memory_used"]} / {cloud["memory_quota"]}'),
                ('SSD space used', f'{cloud["ssd_space_used"]} / {cloud["ssd_space_quota"]}'),
                ('HDD space used', f'{cloud["hdd_space_used"]} / {cloud["hdd_space_quota"]}'),
            )
        )

    print_response(
        ctx,
        get_clouds(ctx, cluster_ids=cluster_ids, limit=limit),
        default_format='table',
        field_formatters=FIELD_FORMATTERS,
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
    )


@cloud_group.command('enable-feature')
@argument('cloud_id')
@argument('feature')
@option('--metadb', is_flag=True)
@pass_context
def enable_feature_command(ctx, cloud_id, feature, metadb):
    """Enable feature for the cloud."""
    if metadb:
        enable_feature_on_cloud(ctx, cloud_id, feature)
    else:
        _enable_iam_feature(ctx, cloud_id, feature)
    echo(f'Feature "{feature}" has been enabled for the cloud "{cloud_id}".')


@cloud_group.command('disable-feature')
@argument('cloud_id')
@argument('feature')
@pass_context
def disable_feature_command(ctx, cloud_id, feature):
    """Enable feature for the cloud."""
    disable_feature_on_cloud(ctx, cloud_id, feature)
    _disable_iam_feature(ctx, cloud_id, feature)
    echo(f'Feature "{feature}" has been disabled for the cloud "{cloud_id}".')


def _get_iam_features(ctx, cloud_id):
    return resourcemanager.get_feature_flags(ctx, cloud_id)


def _enable_iam_feature(ctx, cloud_id, feature):
    url_path = f'/v1/clouds/{cloud_id}:addPermissionStages'
    data = {
        'permissionStages': [feature],
    }
    rest_request(ctx, 'identity', 'POST', url_path, data)


def _disable_iam_feature(ctx, cloud_id, feature):
    url_path = f'/v1/clouds/{cloud_id}:removePermissionStages'
    data = {
        'permissionStages': [feature],
    }
    rest_request(ctx, 'identity', 'POST', url_path, data)


@cloud_group.command('add-quota')
@argument('cloud_id')
@option('--clusters', help='Clusters quota to add.')
@option('--cpu', help='CPU quota to add.')
@option('--gpu', help='GPU quota to add.')
@option('--memory', type=BytesParamType(), help='Memory quota to add.')
@option('--ssd', type=BytesParamType(), help='SSD quota to add.')
@option('--hdd', type=BytesParamType(), help='HDD quota to add.')
@pass_context
def add_quota_command(ctx, cloud_id, clusters, cpu, gpu, memory, ssd, hdd):
    """Add quota to the cloud."""
    update_cloud_quota(
        ctx,
        cloud_id=cloud_id,
        add_clusters=clusters,
        add_cpu=cpu,
        add_gpu=gpu,
        add_memory=memory,
        add_ssd_space=ssd,
        add_hdd_space=hdd,
    )
    echo('Quota updated.')


@cloud_group.command('get-used-resources')
@argument('cloud_id')
@pass_context
def get_used_resources_command(ctx, cloud_id):
    """Get used resources for the cloud."""
    request = cluster_service_pb2.GetUsedResourcesRequest(cloud_ids=[cloud_id])
    service = grpc_service(ctx, 'intapi', cluster_service_pb2_grpc.ClusterServiceStub)
    response = grpc_request(ctx, service.GetUsedResources, request)
    print_response(ctx, response)
