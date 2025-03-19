import click
from cloud.mdb.cli.dbaas.internal import rest
from cloud.mdb.cli.dbaas.internal.grpc import grpc_request
from cloud.mdb.cli.dbaas.internal.metadb.task import update_task, wait_task
from cloud.mdb.cli.dbaas.internal.utils import dry_run_mode


class ClusterType(click.Choice):
    name = 'cluster_type'

    def __init__(self):
        super().__init__(
            ['clickhouse', 'postgresql', 'mongodb', 'redis', 'mysql', 'hadoop', 'elasticsearch', 'kafka', 'opensearch']
        )


def perform_operation_grpc(ctx, method, request, hidden=False):
    response = grpc_request(ctx, method, request)

    if not dry_run_mode(ctx):
        if hidden:
            update_task(ctx, response.id, hidden=hidden)

        wait_task(ctx, response.id)


def rest_request(ctx, method, type, resource_url, data=None, **kwargs):
    url_path = f'/mdb/{type}/v1/{resource_url}'
    return rest.rest_request(ctx, 'intapi', method, url_path, data, **kwargs)


def perform_operation_rest(ctx, method, type, resource_url, *, data=None, hidden=False, **kwargs):
    response = rest_request(ctx, method, type, resource_url, data=data, **kwargs)

    if not dry_run_mode(ctx):
        if hidden:
            update_task(ctx, response['id'], hidden=hidden)

        wait_task(ctx, response['id'])
