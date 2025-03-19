from abc import ABC, abstractmethod
import logging

from flask import current_app
from cloud.mdb.internal.python.compute.iam import jwt
from cloud.mdb.internal.python import grpcutil

from .logs import get_logger


# pylint: disable=too-few-public-methods
class IamJwtProvider(ABC):
    """
    Abstract IAM JWT provider
    """

    @abstractmethod
    def get_iam_token(self):
        """
        Check if service account have role in folder
        """


class YCIamJwtProvider(IamJwtProvider):
    def __init__(self, config: jwt.Config, sa_creds: jwt.SACreds):
        self.iam_jwt = jwt.IamJwt(config, logging.LoggerAdapter(get_logger(), {}), sa_creds)

    def get_iam_token(self):
        return self.iam_jwt.get_token()


def get_provider() -> IamJwtProvider:
    """
    Get resource manager according to config and flags
    """
    iam_jwt_config = current_app.config['IAM_JWT_CONFIG']
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
    return current_app.config['IAM_JWT'](config, sa_creds)
