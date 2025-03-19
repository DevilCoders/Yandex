from click import argument, command, option, pass_context

from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal.intapi import rest_request
from cloud.mdb.cli.dbaas.internal.metadb.cluster import get_cluster_type_by_id


@command('logs')
@argument('cluster_id', metavar='CLUSTER')
@argument('service')
@option('--from-time')
@option('--to-time')
@pass_context
def logs_command(ctx, cluster_id, service, from_time, to_time):
    """Get logs."""
    params = {
        'serviceType': service,
    }
    if from_time:
        params['fromTime'] = from_time
    if to_time:
        params['toTime'] = to_time

    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    response = rest_request(ctx, 'GET', cluster_type, f'clusters/{cluster_id}:logs', params=params)
    print_response(ctx, response)
