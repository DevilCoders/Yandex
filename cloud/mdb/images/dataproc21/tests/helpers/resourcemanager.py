"""
Helpers for working with yandexcloud resource manager
"""

import time
import logging

from retrying import retry
from . import utils

from yandex.cloud.access.access_pb2 import (
    ListAccessBindingsRequest,
    UpdateAccessBindingsRequest,
    AccessBinding,
    AccessBindingDelta,
    AccessBindingAction,
)
from yandex.cloud.resourcemanager.v1.folder_service_pb2_grpc import FolderServiceStub

LOG = logging.getLogger('resourcemanager')


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def _list_access_bindings_page(service, folder_id, page_size, page_token):
    """
    Request for retrying list access bindings
    """
    return service.ListAccessBindings(ListAccessBindingsRequest(
        resource_id=folder_id,
        page_size=page_size,
        page_token=page_token,
    ))


def folder_list_access_bindings(ctx, page_size=1000):
    """
    ListAccessBindings on folder
    """
    conf = ctx.conf
    sdk = ctx.state['yandexsdk']
    folder_id = conf['environment']['folder-id']
    service = sdk.client(FolderServiceStub)
    page_token = None
    ret = []
    while True:
        response = _list_access_bindings_page(service, folder_id, page_size, page_token)
        ret.extend(response.access_bindings)
        page_token = response.next_page_token
        if not page_token:
            break
    return ret


def service_account_get_roles(ctx, service_account_id):
    """
    Return a list of roles for service_account_id in folder
    """
    roles = []
    for access_binding in folder_list_access_bindings(ctx):
        if access_binding.subject.type == 'serviceAccount' and \
                access_binding.subject.id == service_account_id:
            roles.append(access_binding.role_id)
    return roles


def service_account_update_roles(ctx, service_account_id, roles):
    """
    Update roles for serivice_account
    """
    roles = set(roles)
    actual_roles = set(service_account_get_roles(ctx, service_account_id))
    if not (roles - actual_roles):
        LOG.debug(f'Service account `{service_account_id}` already has required roles')
        return
    folder_update_access_bindings(ctx, service_account_id, list(actual_roles | roles))


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def folder_update_access_bindings(ctx, service_account_id, roles):
    """
    UpdateAccessBindings for service_account
    """
    conf = ctx.conf
    sdk = ctx.state['yandexsdk']
    folder_id = conf['environment']['folder-id']
    service = sdk.client(FolderServiceStub)

    deltas = []
    for role in roles:
        deltas.append(AccessBindingDelta(
            action=AccessBindingAction.ADD,
            access_binding=AccessBinding(
                role_id=role,
                subject={'id': service_account_id, 'type': 'serviceAccount'}),
        ))
    req = service.UpdateAccessBindings(
            UpdateAccessBindingsRequest(
                resource_id=folder_id,
                access_binding_deltas=deltas,
            ),
    )
    # wait until operation w/o result completes
    waiter = sdk.waiter(
        req.id,
    )
    for _ in waiter:
        time.sleep(1)
