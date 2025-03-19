from cloud.mdb.cli.dbaas.internal.rest import rest_request


def get_host(ctx, hostname):
    """
    Get host.
    """
    return rest_request(ctx, 'conductor', 'GET', f'/api/v1/hosts/{hostname}')


def delete_host(ctx, hostname, suppress_errors=False):
    """
    Delete host from conductor.
    """
    return rest_request(ctx, 'conductor', 'DELETE', f'/api/v1/hosts/{hostname}', suppress_errors=suppress_errors)
