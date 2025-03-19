from cloud.mdb.cli.dbaas.internal.rest import rest_request


def get_certificate(ctx, hostname):
    """
    Get certificate issued for the specified host.
    """
    return rest_request(ctx, 'certificator', 'GET', '/v1/cert', params={'hostname': hostname})


def delete_certificate(ctx, hostname, suppress_errors=False):
    """
    Delete certificate issued for the specified host.
    """
    return rest_request(
        ctx,
        'certificator',
        'DELETE',
        '/v1/cert',
        params={'hostname': hostname},
        suppress_errors=suppress_errors,
    )
