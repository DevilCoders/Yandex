import time

import requests

import jwt
from click import ClickException

from cloud.mdb.cli.dbaas.internal import vault
from cloud.mdb.cli.dbaas.internal.config import get_config, get_environment_name
from cloud.mdb.cli.dbaas.internal.grpc_stateless import create_grpc_channel
from yandex.cloud.priv.iam.v1 import iam_token_service_pb2, iam_token_service_pb2_grpc


def get_iam_token(ctx, config_section=None, cookie=None):
    """
    Get and return IAM token.
    """
    default_section = 'iam'
    if not config_section:
        config_section = default_section

    if 'iam_tokens' not in ctx.obj:
        ctx.obj['iam_tokens'] = {}

    iam_tokens = ctx.obj['iam_tokens']
    if config_section not in iam_tokens:
        iam_token = None
        for section in (config_section, default_section):
            config = get_config(ctx).get(section, {})
            if cookie is not None:
                iam_tokens[config_section] = _get_iam_token_by_cookie(ctx, cookie)
            elif config.get('iam_token'):
                iam_token = config.get('iam_token')
            elif config.get('token'):
                iam_token = _get_iam_token_by_oauth_token(ctx, config)
            elif config.get('api_key'):
                api_key = vault.get_secret(config.get('api_key'))
                iam_token = _get_iam_token_by_api_key(
                    ctx=ctx,
                    account_id=api_key['service_account_id'],
                    key_id=api_key['id'],
                    private_key=api_key['private_key'],
                )
            elif config.get('user_key'):
                user_key = config['user_key']
                iam_token = _get_iam_token_by_api_key(
                    ctx=ctx,
                    account_id=user_key['user_account_id'],
                    key_id=user_key['id'],
                    private_key=user_key['private_key'],
                )

        if not iam_token:
            raise ClickException(f'IAM is not available in "{get_environment_name(ctx)}".')

        iam_tokens[config_section] = iam_token

    return iam_tokens[config_section]


def _get_iam_token_by_oauth_token(ctx, config):
    iam_config = ctx.obj['config']['iam']
    iam_url = iam_config['rest_endpoint']
    try:
        res = requests.post(
            f'{iam_url}/public/v1/tokens',
            headers={
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            verify=iam_config.get('ca_path', True),
            json={'oauthToken': config['token']},
        )
        res.raise_for_status()
        return res.json()['iamToken']
    except requests.ConnectionError as e:
        raise ClickException(f'Failed to connect to {iam_url}.\n{e}')
    except requests.RequestException as e:
        raise ClickException(f'Failed to get IAM token.\n{e}')


def _get_iam_token_by_api_key(ctx, account_id, key_id, private_key):
    now = int(time.time())
    payload = {
        'aud': ctx.obj['config']['iam']['jwt_audience'],
        'iss': account_id,
        'iat': now,
        'exp': now + 3600,
    }
    jwt_token = jwt.encode(payload, private_key, algorithm='PS256', headers={'kid': key_id})

    request = iam_token_service_pb2.CreateIamTokenRequest(jwt=jwt_token)

    service = iam_token_service_pb2_grpc.IamTokenServiceStub(create_grpc_channel(ctx, 'token_service'))

    return service.Create(request).iam_token


def _get_iam_token_by_cookie(ctx, cookie):
    request = iam_token_service_pb2.CreateIamTokenRequest(iam_cookie=cookie)
    service = iam_token_service_pb2_grpc.IamTokenServiceStub(create_grpc_channel(ctx, 'token_service'))
    return service.Create(request).iam_token
