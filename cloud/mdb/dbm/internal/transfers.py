# -*- encoding: utf-8
"""
Transfers-related functions
"""

from flask import abort, jsonify

from .db import execute


def get_transfers():
    """
    Get transfers (for UI)
    """
    ret = execute('get_transfers')
    return ret


def get_transfer(transfer_id):
    """
    Get transfer by id
    """
    ret = execute('get_transfer', id=transfer_id)
    if not ret:
        res = jsonify({'error': 'No such transfer'})
        res.status_code = 404
        abort(res)

    return ret[0]


def get_transfer_by_container(fqdn):
    """
    Get transfer by container fqdn
    """
    ret = execute('get_transfer_by_container', fqdn=fqdn)
    if not ret:
        return {}

    return ret[0]


def finish_transfer(transfer_id):
    """
    Move original container and remove placeholder
    """
    transfer = get_transfer(transfer_id)

    execute('swap_disks', fetch=False, source=transfer['container'], destination=transfer['placeholder'])
    execute('delete_transfer', fetch=False, id=transfer_id)
    execute('delete_container_volumes', fetch=False, fqdn=transfer['placeholder'])
    execute('unsafe_delete_container', fetch=False, fqdn=transfer['placeholder'])
    execute(
        'move_volumes',
        fetch=False,
        container=transfer['container'],
        src_dom0=transfer['src_dom0'],
        dest_dom0=transfer['dest_dom0'],
    )
    execute(
        'move_container',
        fetch=False,
        container=transfer['container'],
        src_dom0=transfer['src_dom0'],
        dest_dom0=transfer['dest_dom0'],
    )

    return {}


def cancel_transfer(transfer_id):
    """
    Drop placeholder
    """
    transfer = get_transfer(transfer_id)

    execute('delete_transfer', fetch=False, id=transfer_id)
    execute('delete_container_volumes', fetch=False, fqdn=transfer['placeholder'])
    execute('unsafe_delete_container', fetch=False, fqdn=transfer['placeholder'])
    execute('delete_volume_backups', fetch=False, container=transfer['container'], dom0=transfer['src_dom0'], path=None)

    return {}
