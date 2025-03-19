from cloud.mdb.cli.dbaas.internal.rest import rest_request


def set_allow_new_hosts(ctx, dom0_id, value):
    """
    Set allow_new_hosts flag for dom0 server.
    """
    return rest_request(
        ctx,
        'dbm_api',
        'PUT',
        f'/api/v2/dom0/allow_new_hosts/{dom0_id}',
        data={
            'allow_new_hosts': value,
        },
    )


def delete_porto_container(ctx, fqdn):
    """
    Delete porto container.
    """
    return rest_request(ctx, 'dbm_api', 'DELETE', f'/api/v2/containers/{fqdn}')


def finish_transfer(ctx, transfer_id):
    """
    Finish porto container transfer.
    """
    return rest_request(ctx, 'dbm_api', 'POST', f'/api/v2/transfers/{transfer_id}/finish')


def cancel_transfer(ctx, transfer_id):
    """
    Cancel porto container transfer.
    """
    return rest_request(ctx, 'dbm_api', 'POST', f'/api/v2/transfers/{transfer_id}/cancel')
