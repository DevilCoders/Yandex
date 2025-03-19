"""
IAM JWT token getter with cache
"""

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.iam import jwt

from cloud.mdb.dbaas_worker.internal.providers.common import BaseProvider


class IamJwt(BaseProvider):
    """
    IAM JWT token provider
    """

    def __init__(
        self, config, task, queue, service_account_id: str = None, key_id: str = None, private_key: str = None
    ):
        """
        It use default (worker) SA creds if service_account_id is None
        """
        super().__init__(config, task, queue)
        if not service_account_id:
            sa_creds = jwt.SACreds(
                service_account_id=self.config.iam_jwt.service_account_id,
                key_id=self.config.iam_jwt.key_id,
                private_key=self.config.iam_jwt.private_key,
            )
        else:
            sa_creds = jwt.SACreds(
                service_account_id=service_account_id,
                key_id=key_id,  # type: ignore
                private_key=private_key,  # type: ignore
            )

        self._iam_jwt = jwt.IamJwt(
            config=jwt.Config(
                transport=grpcutil.Config(
                    server_name=self.config.iam_jwt.server_name,
                    url=self.config.iam_jwt.url,
                    cert_file=self.config.iam_jwt.cert_file,
                    insecure=self.config.iam_jwt.insecure,
                ),
                audience=self.config.iam_jwt.audience,
                request_expire=self.config.iam_jwt.request_expire,
                expire_thresh=self.config.iam_jwt.expire_thresh,
            ),
            logger=self.logger,
            sa_creds=sa_creds,
        )

    def get_token(self):
        """
        Get iam token
        """
        return self._iam_jwt.get_token()
