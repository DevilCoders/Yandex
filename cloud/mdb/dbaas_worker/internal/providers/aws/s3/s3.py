from ....exceptions import ExposedException
from ...common import BaseProvider, Change
from ..sts import get_assume_role_session

from dbaas_common import retry, tracing

from botocore.config import Config
from botocore.exceptions import ClientError

import json


class AWSS3APIError(ExposedException):
    """
    Base S3 error
    """


class AWSS3DisabledError(RuntimeError):
    """
    S3 provider not initialized. Enable it in config'
    """


class AWSS3(BaseProvider):
    _s3 = None

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        if config.aws_s3.enabled:
            session = get_assume_role_session(
                config=config,
                task=task,
                queue=queue,
                aws_access_key_id=config.aws.access_key_id,
                aws_secret_access_key=config.aws.secret_access_key,
                region_name=config.aws.region_name,
            )

            self._s3 = session.client(  # type: ignore
                's3',
                config=Config(
                    retries=config.aws_s3.retries,
                ),
            )

    @retry.on_exception(ClientError, factor=10, max_wait=60, max_tries=6)
    def _put_bucket_policy(self, bucket_name, policy):
        str_policy = json.dumps(policy)
        self._s3.put_bucket_policy(Bucket=bucket_name, Policy=str_policy)  # type: ignore

    @tracing.trace('Grant user access to S3 bucket')
    def _grant_access(self, bucket_name: str, user_arn: str) -> None:
        tracing.set_tag('iam.user_arn', user_arn)
        tracing.set_tag('s3.bucket', bucket_name)
        policy = {
            'Version': '2012-10-17',
            'Statement': [
                {
                    'Sid': 'AllowAccessFromClusterUser',
                    'Effect': 'Allow',
                    'Principal': {'AWS': user_arn},
                    'Action': ['s3:*'],
                    'Resource': [f'arn:aws:s3:::{bucket_name}', f'arn:aws:s3:::{bucket_name}/*'],
                }
            ],
        }
        self._put_bucket_policy(bucket_name=bucket_name, policy=policy)
        self.logger.debug('Grant for user %r access to bucket %r', user_arn, bucket_name)

    @tracing.trace('Grant user access to S3 bucket')
    def grant_access(self, bucket_name: str, user_arn: str) -> None:
        if self._s3 is None:
            raise AWSS3DisabledError
        self.add_change(
            Change(
                f'iam.user.{user_arn}',
                'grant access',
            )
        )
        self._grant_access(bucket_name=bucket_name, user_arn=user_arn)
