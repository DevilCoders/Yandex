"""
Helpers for working with yandexcloud sdk
"""

import grpc
import logging

from retrying import retry
from . import utils

from yandex.cloud.iam.v1.service_account_pb2 import ServiceAccount
from yandex.cloud.iam.v1.service_account_service_pb2 import (
    CreateServiceAccountRequest,
    CreateServiceAccountMetadata,
    ListServiceAccountsRequest,
    DeleteServiceAccountRequest,
    DeleteServiceAccountMetadata,
)
from yandex.cloud.iam.v1.service_account_service_pb2_grpc import ServiceAccountServiceStub

from yandex.cloud.iam.v1.awscompatibility.access_key_service_pb2 import (
    CreateAccessKeyRequest,
)
from yandex.cloud.iam.v1.awscompatibility.access_key_service_pb2_grpc import AccessKeyServiceStub


LOG = logging.getLogger('iam')


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def service_account_create(ctx, name, description=None):
    """
    Create IAM service account
    """
    conf = ctx.conf
    sdk = ctx.state['yandexsdk']
    folder_id = conf['environment']['folder-id']
    service = sdk.client(ServiceAccountServiceStub)
    try:
        operation = service.Create(CreateServiceAccountRequest(
            folder_id=folder_id,
            name=name,
            description=description,
        ))
        operation_result = sdk.wait_operation_and_get_result(
            operation,
            response_type=ServiceAccount,
            meta_type=CreateServiceAccountMetadata,
            logger=LOG,
        )
        service_account_id = operation_result.response.id
        ctx.state['resources']['service_accounts'][name] = service_account_id
    except grpc.RpcError:
        raise
    return service_account_id


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def service_account_delete(ctx, name):
    """
    Delete IAM service-account
    """
    sdk = ctx.state['yandexsdk']
    service_account_id = ctx.state['resources']['service_accounts'].get(name)
    if not service_account_id:
        service_account_id = next(x.id for x in service_accounts_list(ctx) if x.name == name)
        if not service_account_id:
            LOG.warn(f'iam service-account {name} not found, can not delete it')
            return
    service = sdk.client(ServiceAccountServiceStub)
    try:
        operation = service.Delete(DeleteServiceAccountRequest(
            service_account_id=service_account_id,
        ))
        sdk.wait_operation_and_get_result(
            operation,
            response_type=ServiceAccount,
            meta_type=DeleteServiceAccountMetadata,
            logger=LOG,
        )
    except grpc.RpcError as err:
        code = utils.extract_code(err)
        if code == grpc.StatusCode.NOT_FOUND:
            LOG.warn(f'Service account {service_account_id} already deleted')
        else:
            raise
    finally:
        ctx.state['resources']['service_accounts'].pop(name, None)


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def _list_service_accounts_page(service, folder_id, page_size, page_token):
    """
    Request for retrying
    """
    return service.List(ListServiceAccountsRequest(
        folder_id=folder_id,
        page_size=page_size,
        page_token=page_token,
    ))


def service_accounts_list(ctx, page_size=1000):
    """
    List all service accounts
    """
    sdk = ctx.state['yandexsdk']
    folder_id = ctx.conf['environment']['folder-id']
    page_token = None
    service = sdk.client(ServiceAccountServiceStub)
    ret = []
    while True:
        response = _list_service_accounts_page(service, folder_id, page_size, page_token)
        ret.extend(response.service_accounts)
        page_token = response.next_page_token
        if not page_token:
            break
    return ret


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def service_account_create_aws_credentials(ctx, service_account_id, description=None):
    """
    Create AWS compatbile credentials.
    Needs for working with object storage api.
    """
    sdk = ctx.state['yandexsdk']
    service = sdk.client(AccessKeyServiceStub)
    try:
        resp = service.Create(CreateAccessKeyRequest(
            service_account_id=service_account_id,
            description=description,
        ))
        return (resp.access_key.key_id, resp.secret)
    except grpc.RpcError:
        raise
