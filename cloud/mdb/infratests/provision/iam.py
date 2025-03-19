import logging

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.iam import jwt
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from cloud.mdb.infratests.config import InfratestConfig


def create_iam_jwt(config: InfratestConfig, logger: logging.Logger) -> jwt.IamJwt:
    sa = config.provisioner_service_account
    sa_creds = jwt.SACreds(
        service_account_id=sa.id,
        key_id=sa.key_id,
        private_key=sa.private_key,
    )

    jwt_config = config.iam_jwt
    return jwt.IamJwt(
        config=jwt.Config(
            transport=grpcutil.Config(
                server_name=jwt_config.server_name,
                url=jwt_config.url,
                cert_file=jwt_config.cert_file,
                insecure=jwt_config.insecure,
            ),
            audience=jwt_config.audience,
            request_expire=jwt_config.request_expire,
            expire_thresh=jwt_config.expire_thresh,
        ),
        logger=MdbLoggerAdapter(logger, extra={}),
        sa_creds=sa_creds,
    )
