import logging

from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.iam.jwt import IamJwt, SACreds, Config


def get_iam_client(config) -> IamJwt:
    sa_cfg = config['app']['service_account']
    sa_creds = SACreds(
        service_account_id=sa_cfg['id'],
        key_id=sa_cfg['key_id'],
        private_key=sa_cfg['private_key'],
    )
    return IamJwt(
        config=Config(
            transport=grpcutil.Config(
                url=config['app']['environment']['services']['iam']['v1']['token_service']['endpoint'],
                cert_file=config['cert_file'],
            ),
            audience='https://iam.api.cloud.yandex.net/iam/v1/tokens',
            request_expire=3600,
            expire_thresh=180,
        ),
        logger=MdbLoggerAdapter(logging.getLogger(__name__), {}),
        sa_creds=sa_creds,
    )
