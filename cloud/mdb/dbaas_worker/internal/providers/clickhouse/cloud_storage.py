"""
ClickHouse cloud storage feature manipulation
"""
from functools import cached_property

from dbaas_common import tracing
from ..common import BaseProvider
from ..iam import Iam
from ..iam_jwt import IamJwt
from ..aws.iam import AWSIAM, DOUBLE_CLOUD_CLUSTERS_IAM_PATH
from ..aws.s3 import AWSS3
from ..pillar import DbaasPillar
from ..s3_bucket import S3Bucket
from ...crypto import encrypt


class CloudStorage(BaseProvider):
    """
    ClickHouse cloud storage provider
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue)
        self.source_bucket_name = self._source_cloud_storage_bucket(args)
        self.target_bucket_name = self._cloud_storage_bucket(args)
        self.iam_jwt = IamJwt(
            config,
            task,
            queue,
            config.cloud_storage.service_account_id,
            config.cloud_storage.key_id,
            config.cloud_storage.private_key,
        )  # Issue token
        self.iam = Iam(config, task, queue, iam_jwt=self.iam_jwt)  # Create service account and S3 credentials
        self.s3_bucket = S3Bucket(config, task, queue)
        self.pillar = DbaasPillar(config, task, queue)
        self.aws_iam = AWSIAM(self.config, self.task, self.queue)

    @cached_property
    def aws_s3(self) -> AWSS3:
        return AWSS3(self.config, self.task, self.queue)

    @staticmethod
    def _cloud_storage_bucket(task_args) -> str:
        return task_args.get('s3_buckets', {}).get('cloud_storage', None)

    @staticmethod
    def _source_cloud_storage_bucket(task_args) -> str:
        return task_args.get('source_cloud_storage_s3_bucket', None)

    def has_source_cloud_storage_bucket(self) -> bool:
        """
        Check whether task has source cloud storage bucket or not.
        """
        return self.source_bucket_name is not None

    def has_cloud_storage_bucket(self) -> bool:
        """
        Check whether task has cloud storage bucket or not.
        """
        return self.target_bucket_name is not None

    def has_exists_cloud_storage_bucket(self, subcid: str) -> bool:
        """
        Check when cluster already has cloud storage bucket.
        """
        return not not self.pillar.get('subcid', subcid, ['data', 'cloud_storage', 's3', 'bucket'])

    def _build_service_account_name(self) -> str:
        return 'cloud-storage-' + self.task['cid']

    @tracing.trace('Cloud storage protect by service account')
    def protect(self, subcid: str):
        """
        Protect access to cloud storage bucket with separate S3 credentials.
        """

        self.iam.reconnect()

        sa_id = self.iam.create_service_account(
            self.config.cloud_storage.folder_id,
            self._build_service_account_name(),
        )
        access_key, secret_key = self.iam.create_access_key(sa_id)

        self.pillar.exists(
            'subcid',
            subcid,
            ['data', 'cloud_storage', 's3'],
            ['access_key_id', 'access_secret_key'],
            [encrypt(self.config, access_key), encrypt(self.config, secret_key)],
        )

        if self.has_source_cloud_storage_bucket():
            self.s3_bucket.grant_full_access_to_service_account(
                bucket_name=self.source_bucket_name,
                service_account_id=sa_id,
                endpoint='cloud_storage',
            )

        self.s3_bucket.grant_full_access_to_service_account(
            bucket_name=self.target_bucket_name,
            service_account_id=sa_id,
            endpoint='cloud_storage',
        )

    @tracing.trace('Cloud storage service account absent')
    def service_account_absent(self):
        """
        Make sure service account for cloud storage is deleted.
        """
        self.iam.reconnect()

        service_account = self.iam.find_service_account_by_name(
            self.config.cloud_storage.folder_id,
            self._build_service_account_name(),
        )

        if service_account:
            self.iam.delete_service_account(service_account.id)

    def set_billing_tag(self):
        tags = {
            'yc_mdb_chs3_payer': self.task['folder_id'],
        }
        self.s3_bucket.set_system_tags(self.target_bucket_name, tags)

    def aws_protect(self, ch_subcid):
        enc_current_access_key = self.pillar.get(
            'subcid', ch_subcid, ['data', 'cloud_storage', 's3', 'access_key_id', 'data']
        )
        if not enc_current_access_key:
            user_name = 'cluster-user-' + self.task['cid']
            self.aws_iam.create_user(user_name, DOUBLE_CLOUD_CLUSTERS_IAM_PATH)
            user_arn = self.aws_iam.get_user_arn(user_name)
            self.aws_s3.grant_access(self.target_bucket_name, user_arn)
            access_key, secret_key = self.aws_iam.create_access_key(user_name)
            self.pillar.exists(
                'subcid',
                ch_subcid,
                ['data', 'cloud_storage', 's3'],
                ['access_key_id', 'access_secret_key', 'scheme', 'endpoint'],
                [
                    encrypt(self.config, access_key),
                    encrypt(self.config, secret_key),
                    'https',
                    f's3.{self.config.aws.region_name}.amazonaws.com',
                ],
            )
