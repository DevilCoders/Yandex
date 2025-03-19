"""
Tool for getting IAM token
"""

import logging

from cloud.mdb.internal.python.compute.iam import jwt
from cloud.mdb.internal.python import grpcutil


def get_logger():
    return logging.getLogger(__name__)


def get_iam_token(sa_key, verify=None):
    """
    Gets IAM token from env or from IAM via service account key
    """

    iam_jwt_config = {
        'url': 'ts.private-api.cloud-preprod.yandex.net:4282',
        'cert_file': '/opt/yandex/allCAs.pem',
        'server_name': 'ts.private-api.cloud-preprod.yandex.net',
        'service_account_id': sa_key['service_account_id'],
        'key_id': sa_key['id'],
        'private_key': sa_key['private_key'],
        'insecure': False,
        'audience': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
        'expire_thresh': 180,
        'request_expire': 3600,
    }
    config = jwt.Config(
        transport=grpcutil.Config(
            url=iam_jwt_config['url'],
            cert_file=iam_jwt_config.get('cert_file'),
            server_name=iam_jwt_config.get('server_name'),
            insecure=iam_jwt_config.get('insecure'),
        ),
        audience=iam_jwt_config['audience'],
        request_expire=iam_jwt_config['request_expire'],
        expire_thresh=iam_jwt_config['expire_thresh'],
    )
    sa_creds = jwt.SACreds(
        service_account_id=iam_jwt_config['service_account_id'],
        key_id=iam_jwt_config['key_id'],
        private_key=iam_jwt_config['private_key'],
    )
    iam_jwt = jwt.IamJwt(config, logging.LoggerAdapter(get_logger(), {}), sa_creds)
    return iam_jwt.get_token()
