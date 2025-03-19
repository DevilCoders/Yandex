import logging
from django.conf import settings

import cloud.mdb.internal.python.logs as logs
import cloud.mdb.internal.python.compute.iam.jwt as jwt
import cloud.mdb.internal.python.grpcutil as grpcutil


logger = logs.MdbLoggerAdapter(logging.getLogger(__name__), extra={})


class TokenGetter:
    def __init__(self):
        self.jwt = None

    def get_token(self):
        if self.jwt is None:
            self.init_jwt()
        return self.jwt.get_token()

    def init_jwt(self):
        with open(settings.CONFIG.iam.sa.key) as fd:
            private_data = fd.read()
        self.jwt = jwt.IamJwt(
            config=jwt.Config(
                transport=grpcutil.Config(
                    server_name=settings.CONFIG.iam.token_service.host,
                    url=f'{settings.CONFIG.iam.token_service.host}:{settings.CONFIG.iam.token_service.port}',
                    cert_file=None,
                ),
                audience='https://iam.api.cloud.yandex.net/iam/v1/tokens',
                request_expire=3600,
                expire_thresh=180,
            ),
            logger=logger,
            sa_creds=jwt.SACreds(
                service_account_id=settings.CONFIG.iam.sa.id,
                key_id=settings.CONFIG.iam.sa.key_id,
                private_key=private_data,
            ),
        )
