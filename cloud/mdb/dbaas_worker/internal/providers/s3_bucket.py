"""
S3 Bucket handle module
"""
import boto3
import urllib3
from botocore.client import Config
from botocore.exceptions import ClientError

from dbaas_common import retry, tracing
from .aws.sts import get_assume_role_session
from .common import BaseProvider, Change
from .http import HTTPClient
from .iam_jwt import IamJwt
from ..exceptions import ExposedException


class S3OperationError(ExposedException):
    """
    Base S3 operation exception
    """


S3_RETRIES = retry.on_exception(
    (S3OperationError, ClientError, boto3.exceptions.Boto3Error), factor=10, max_wait=60, max_tries=10
)


class S3SystemTagsClient(HTTPClient):
    """
    Class for setting system tags for S3 bucket
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self._endpoint = None
        if self.config.s3.idm_endpoint_url:
            self._endpoint = self.config.s3.idm_endpoint_url
        if not self._disabled:
            iam = IamJwt(config, task, queue)
            headers = {
                'X-YaCloud-SubjectToken': iam.get_token(),
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            }
            self._init_session(self._endpoint, default_headers=headers)

    @property
    def _disabled(self) -> bool:
        return not self._endpoint

    @S3_RETRIES
    @tracing.trace('S3 Set bucket system tags')
    def _set_system_tags(self, name, tags):
        """
        Set system tags to bucket implementation
        """
        tracing.set_tag('s3.bucket.system_tags', tags)

        data = {"tags": tags}
        url = f'/management/bucket/{name}/tags/'
        self._make_request(url, method='put', data=data)

    def set_system_tags(self, name, tags):
        if not self._disabled:
            self._set_system_tags(name, tags)


class S3Bucket(BaseProvider):
    """
    S3 Bucket provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.system_tags_client = S3SystemTagsClient(config, task, queue)

    def _get_s3_client(self, endpoint):
        endpoint_config = self.config.s3.__dict__[endpoint]
        verify = True
        if self.config.s3.ca_path:
            verify = self.config.s3.ca_path
        create_kwargs = {
            'verify': verify,
        }
        # Treat unset endpoint_url as AWS
        endpoint_url = endpoint_config['endpoint_url']
        if endpoint_url:
            create_kwargs['endpoint_url'] = endpoint_url
        if self.config.s3.addressing_style:
            create_kwargs['config'] = Config(s3={'addressing_style': self.config.s3.addressing_style})
        if self.config.aws.dataplane_role_arn:
            session = get_assume_role_session(
                config=self.config,
                task=self.task,
                queue=self.queue,
                aws_access_key_id=endpoint_config['access_key_id'],
                aws_secret_access_key=endpoint_config['secret_access_key'],
                region_name=self.config.s3.region_name,
            )
        else:
            session = boto3.Session(
                aws_access_key_id=endpoint_config['access_key_id'],
                aws_secret_access_key=endpoint_config['secret_access_key'],
                region_name=self.config.s3.region_name,
            )
        client = session.resource(
            's3',
            **create_kwargs,
        )
        if self.config.s3.disable_ssl_warnings:
            urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
        return client

    @staticmethod
    def _bucket_exists(client, name):
        try:
            client.meta.client.head_bucket(Bucket=name)
        except ClientError as error:
            try:
                if int(error.response['Error']['Code']) == 404:
                    return False
            except ValueError:
                # Some errors have non-digit code.
                # ValueError: invalid literal for int() with base 10: AccessDenied
                pass
            raise error
        return True

    def _eval_create_bucket_options(self, endpoint):
        create_options = {}
        location_constraint = self.config.s3.__dict__[endpoint]['location_constraint']
        if location_constraint:
            # https://docs.aws.amazon.com/AmazonS3/latest/API/API_CreateBucket.html
            # LocationConstraint -> (string)
            # Specifies the Region where the bucket will be created.
            # If you don't specify a Region, the bucket is created in the US East (N. Virginia) Region (us-east-1).
            create_options['CreateBucketConfiguration'] = {
                'LocationConstraint': location_constraint,
            }
        return create_options

    @staticmethod
    def add_lifecycle_policy(client, bucket_name):
        client.meta.client.put_bucket_lifecycle_configuration(
            Bucket=bucket_name,
            LifecycleConfiguration={
                'Rules': [
                    {
                        'ID': 'MDB auto abort multipart upload TTL',
                        "Filter": {"Prefix": ""},
                        'Status': 'Enabled',
                        'AbortIncompleteMultipartUpload': {'DaysAfterInitiation': 7},
                    },
                ]
            },
        )

    @S3_RETRIES
    @tracing.trace('S3 Bucket Exists')
    def _exists(self, client, name, create_options, allow_lifecycle_policies):
        """
        Retryable and idempotent bucket creation
        """
        tracing.set_tag('s3.bucket.name', name)

        if not self._bucket_exists(client, name):
            client.create_bucket(Bucket=name, **create_options)  # pylint: disable=no-member
            if self.config.aws.labels:
                client.BucketTagging(name).put(
                    Tagging={'TagSet': [{'Key': k, 'Value': v} for k, v in self.config.aws.labels.items()]}
                )
        if allow_lifecycle_policies:
            self.add_lifecycle_policy(client, name)

    def exists(self, name, endpoint='backup'):
        """
        Create bucket
        """
        client = self._get_s3_client(endpoint)
        self.add_change(
            Change(f's3_bucket.{name}', 'created', rollback=lambda task, safe_refision: self._absent(client, name))
        )
        allow_lifecycle_policies = self.config.s3.__dict__[endpoint].get('allow_lifecycle_policies', False)
        self._exists(client, name, self._eval_create_bucket_options(endpoint), allow_lifecycle_policies)

    def set_system_tags(self, name, tags: dict):
        """
        Set system tags to bucket
        """
        if self.system_tags_client:
            self.system_tags_client.set_system_tags(name, tags)

    def _delete_objects(self, bucket, delete_list):
        res = bucket.delete_objects(Delete={'Objects': delete_list})
        if res.get('Errors'):
            for error in res['Errors']:
                self.logger.error('S3 deletion error: %s', error)
            raise S3OperationError('Unable to delete {num} objects'.format(num=len(delete_list)))

    @S3_RETRIES
    @tracing.trace('S3 Objects delete')
    def _ensure_bucket_empty(self, client, bucket_name, prefix=''):
        """
        Remove all objects starting with given prefix from bucket
        """
        tracing.set_tag('s3.bucket.name', bucket_name)
        tracing.set_tag('s3.prefix', prefix)

        self.add_change(Change(f's3_path.{prefix}', 'objects removed'))
        bucket = client.Bucket(bucket_name)  # pylint: disable=no-member

        delete_list = []
        count = 0

        for obj in bucket.objects.filter(Prefix=prefix):
            delete_list.append({'Key': obj.key})
            count += 1
            # Multiple delete allows maximum 1000 objects
            if len(delete_list) == 1000:
                self._delete_objects(bucket, delete_list)
                delete_list = []

        if delete_list:
            self._delete_objects(bucket, delete_list)

    @staticmethod
    def _abort_multipart_uploads(client, name):
        """
        Abort all multipart uploads for bucket
        Because boto3 is retarded we are not implementing pagination here
        """
        finish = False
        while not finish:
            resp = client.meta.client.list_multipart_uploads(Bucket=name)
            for upload in resp.get('Uploads', []):
                client.meta.client.abort_multipart_upload(Bucket=name, Key=upload['Key'], UploadId=upload['UploadId'])
            if not (resp.get('NextKeyMarker') or resp.get('NextUploadIdMarker')):
                finish = True

    @S3_RETRIES
    @tracing.trace('S3 Bucket Absent')
    def _absent(self, client, name: str):
        """
        Retryable and idempotent bucket removal
        """
        tracing.set_tag('s3.bucket.name', name)

        if self._bucket_exists(client, name):
            self._abort_multipart_uploads(client, name)
            self._ensure_bucket_empty(client, name)
            client.meta.client.delete_bucket(Bucket=name)

    def absent(self, name, endpoint='backup'):
        """
        Delete bucket
        """
        client = self._get_s3_client(endpoint)
        self.add_change(Change(f's3_bucket.{name}', 'deleted'))
        self._absent(client, name)

    def grant_full_access_to_service_account(self, bucket_name, service_account_id, endpoint='backup'):
        """
        Grants full control to specified service account for bucket.
        It is allowed to pass multiple service accounts as a list of their IDs.
        """
        self.grant_access_to_service_account(
            bucket_name=bucket_name, service_account_id=service_account_id, perm='full', endpoint=endpoint
        )

    @S3_RETRIES
    @tracing.trace('Grant access for S3 bucket')
    def _grant_access_to_service_accounts(self, client, bucket_name: str, service_account_id: str, perm: str):
        tracing.set_tag('s3.bucket.name', bucket_name)
        tracing.set_tag('s3.bucket.perm', perm)

        acl = client.BucketAcl(bucket_name)
        acl.load()

        if perm not in ['full', 'ro', 'rw']:
            raise RuntimeError(f'unknown s3 bucket access permission "{perm}"')

        if perm == 'full':
            self._append_premission(acl.grants, service_account_id, "FULL_CONTROL")
        if perm == 'ro' or perm == 'rw':
            self._append_premission(acl.grants, service_account_id, "READ")
        if perm == 'rw':
            self._append_premission(acl.grants, service_account_id, "WRITE")

        acl.put(
            AccessControlPolicy={
                "Grants": acl.grants,
                "Owner": acl.owner,
            }
        )

    def _append_premission(self, grants, sa_id: str, permission: str):
        grants.append({"Grantee": {"ID": sa_id, "Type": "CanonicalUser"}, "Permission": permission})

    def grant_access_to_service_account(
        self, bucket_name: str, service_account_id: str, perm: str, endpoint: str = 'backup'
    ):
        """
        Grants access to specified service account for bucket.
        It is allowed to pass multiple service accounts as a list of their IDs.
        """
        client = self._get_s3_client(endpoint)

        self.add_change(Change(f'service_account.{service_account_id}', f'has {perm} access to bucket {bucket_name}'))

        self._grant_access_to_service_accounts(client, bucket_name, service_account_id, perm)
