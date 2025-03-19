from collections import OrderedDict

from click import argument, option
from click import group, pass_context
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.common.parameters import ListParamType
from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.dbaas.internal.config import config_option
from cloud.mdb.cli.dbaas.internal.grpc import grpc_service, grpc_request
from cloud.mdb.cli.dbaas.internal.iam import get_iam_token
from yandex.cloud.priv.iam.v1.service_account_service_pb2 import (
    DeleteServiceAccountRequest,
    GetServiceAccountRequest,
    ListServiceAccountsRequest,
)
from yandex.cloud.priv.iam.v1.service_account_service_pb2_grpc import ServiceAccountServiceStub
from yandex.cloud.priv.servicecontrol.v1.access_service_pb2 import AuthenticateRequest, AuthorizeRequest
from yandex.cloud.priv.servicecontrol.v1.access_service_pb2_grpc import AccessServiceStub
from yandex.cloud.priv.servicecontrol.v1.resource_pb2 import Resource


@group('iam')
def iam_group():
    """IAM commands."""
    pass


@iam_group.command('create-token')
@option('--type', 'token_type')
@option('--cookie', 'cookie')
@pass_context
def create_token_command(ctx, token_type, cookie):
    """Create IAM token."""
    print(get_iam_token(ctx, token_type, cookie))


@iam_group.command('check-token')
@argument('token', metavar='IAM_TOKEN')
@pass_context
def check_token_command(ctx, token):
    """Check IAM token."""
    if not token:
        token = get_iam_token(ctx)

    request = AuthenticateRequest(iam_token=token)

    service = grpc_service(ctx, 'access_service', AccessServiceStub)
    print_response(ctx, grpc_request(ctx, service.Authenticate, request))


@iam_group.command('check-permission')
@argument('permission')
@argument('resource_type')
@argument('resource_id')
@option('--iam-token', 'token')
@pass_context
def check_permission_command(ctx, permission, resource_type, resource_id, token):
    """Check permission."""
    if not token:
        token = get_iam_token(ctx)

    request = AuthorizeRequest(
        iam_token=token, permission=permission, resource_path=[Resource(type=resource_type, id=resource_id)]
    )

    service = grpc_service(ctx, 'access_service', AccessServiceStub)
    grpc_request(ctx, service.Authorize, request)


@iam_group.group('sa')
def sa_group():
    """Service account management commands."""
    pass


@sa_group.command('get')
@argument('sa_id', metavar='ID')
@pass_context
def get_service_account_command(ctx, sa_id):
    """Get service account."""
    print_response(ctx, _get_service_account(ctx, sa_id))


@sa_group.command('list')
@option('-f', '--folder', 'folder_id', help='Folder ID.')
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only object IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_service_accounts_command(ctx, folder_id, limit, quiet, separator):
    """List service accounts."""
    if not folder_id:
        folder_id = config_option(ctx, 'compute', 'folder')

    def _table_formatter(sa):
        return OrderedDict(
            (
                ('id', sa['id']),
                ('name', sa['name']),
                ('created', sa['createdAt']),
            )
        )

    print_response(
        ctx,
        _get_service_accounts(ctx, folder_id),
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
        limit=limit,
    )


@sa_group.command('delete')
@argument('sa_ids', metavar='IDS', type=ListParamType())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def delete_service_account_command(ctx, sa_ids, force):
    """Delete one or several service accounts."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    for sa_id in sa_ids:
        _delete_service_account(ctx, sa_id)
        print(f'Service account "{sa_id}" deleted')


def _get_service_account(ctx, service_account_id):
    request = GetServiceAccountRequest(service_account_id=service_account_id)

    service = grpc_service(ctx, 'iam', ServiceAccountServiceStub)

    return grpc_request(ctx, service.Get, request)


def _get_service_accounts(ctx, folder_id):
    request = ListServiceAccountsRequest(folder_id=folder_id)

    service = grpc_service(ctx, 'iam', ServiceAccountServiceStub)

    return grpc_request(ctx, service.List, request).service_accounts[:]


def _delete_service_account(ctx, service_account_id):
    request = DeleteServiceAccountRequest(service_account_id=service_account_id)

    service = grpc_service(ctx, 'iam', ServiceAccountServiceStub)

    return grpc_request(ctx, service.Delete, request)
