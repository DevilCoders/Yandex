"""
S3 Bucket access protection module
"""
from dbaas_common import tracing
from ..crypto import encrypt
from .common import BaseProvider
from .iam_jwt import IamJwt
from .iam import Iam
from .s3_bucket import S3Bucket
from .pillar import DbaasPillar


class S3BucketAccess(BaseProvider):
    """
    S3 Bucket provider with access protected by separate creds (keys/tokens)
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.s3_bucket = S3Bucket(config, task, queue)
        self.pillar = DbaasPillar(config, task, queue)

    def _get_iam_client(self, cfg):
        iam_jwt = IamJwt(
            self.config,
            self.task,
            self.queue,
            cfg['service_account_id'],
            cfg['key_id'],
            cfg['private_key'],
        )  # Issue token
        iam = Iam(self.config, self.task, self.queue, iam_jwt=iam_jwt)  # Create service account and S3 credentials
        iam.reconnect()
        return iam

    def creds_exist(self, endpoint):
        """
        Provide creds to access S3 bucket with requested permissions.
        !Currently only cloud S3 supported via service_account with static keys
        """
        config = self.config.s3.__dict__[endpoint]
        if not config.get('secured'):
            return

        return self._creds_exist(endpoint, config)

    @tracing.trace('Provide s3 bucket access creds')
    def _creds_exist(self, endpoint, config):
        iam = self._get_iam_client(config.get('iam'))

        service_account_name = 'sa-' + self.task['cid']

        service_account_id = iam.create_service_account(config['folder_id'], service_account_name)

        access_key, secret_key = iam.create_access_key(service_account_id)

        key_path = ['data', 's3'] if config['override_default'] else ['data', 's3', 'buckets', endpoint]

        self.pillar.exists(
            'cid',
            self.task['cid'],
            key_path,
            ['endpoint', 'access_key_id', 'access_secret_key', 'service_account_id'],
            [
                config['endpoint_url'],
                encrypt(self.config, access_key),
                encrypt(self.config, secret_key),
                service_account_id,
            ],
        )

    def creds_absent(self, endpoint):
        """
        Make sure creds for accessing s3 bucket is deleted.

        !Currently only cloud S3 supported via service_account with static keys
        """
        config = self.config.s3.__dict__[endpoint]
        if not config.get('secured'):
            return

        return self._creds_absent(config)

    @tracing.trace('S3 bucket access creds absent')
    def _creds_absent(self, config):
        iam = self._get_iam_client(config.get('iam'))

        service_account_name = 'sa-' + self.task['cid']

        service_account = iam.find_service_account_by_name(config['folder_id'], service_account_name)

        if service_account:
            iam.delete_service_account(service_account.id)

    def grant_access(self, bucket, perm, endpoint):
        config = self.config.s3.__dict__[endpoint]
        if not config.get('secured'):
            return

        return self._grant_access(bucket, perm, endpoint, config)

    @tracing.trace('Grant access to S3 bucket')
    def _grant_access(self, bucket, perm, endpoint, config):
        iam = self._get_iam_client(config.get('iam'))

        service_account_name = 'sa-' + self.task['cid']

        service_account = iam.find_service_account_by_name(config['folder_id'], service_account_name)

        if not service_account:
            raise RuntimeError(f'service account not found {service_account_name}')

        self.s3_bucket.grant_access_to_service_account(bucket, service_account.id, perm, endpoint)
