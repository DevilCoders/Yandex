import os
import uuid
import boto3
from typing import Tuple


class Environment:
    @property
    def yc_ai_token(self) -> str:
        if self._yc_ai_token is None:
            return os.environ.get('YC_AI_TOKEN', default=None)
        return self._yc_ai_token

    @yc_ai_token.setter
    def yc_ai_token(self, token: str):
        self._yc_ai_token = token

    @property
    def azure_token(self) -> str:
        if self._azure_token is None:
            return os.environ.get('AZURE_TOKEN', default=None)
        return self._azure_token

    @azure_token.setter
    def azure_token(self, token: str):
        self._azure_token = token

    @property
    def azure_region(self) -> str:
        if self._azure_region is None:
            return os.environ.get('AZURE_REGION', default=None)
        return self._azure_region

    @azure_region.setter
    def azure_region(self, region: str):
        self._azure_region = region

    @property
    def s3_access_key(self) -> str:
        if self._s3_access_key is None:
            return os.environ.get('AWS_ACCESS_KEY_ID', default=None)
        return self._s3_access_key

    @s3_access_key.setter
    def s3_access_key(self, key: str):
        self._s3_access_key = key

    @property
    def s3_secret_access_key(self) -> str:
        if self._s3_secret_access_key is None:
            return os.environ.get('AWS_SECRET_ACCESS_KEY', default=None)
        return self._s3_secret_access_key

    @s3_secret_access_key.setter
    def s3_secret_access_key(self, key: str):
        self._s3_secret_access_key = key

    @property
    def s3_temp_bucket(self) -> str:
        if self._s3_temp_bucket is None:
            return os.environ.get('AWS_TEMP_BUCKET', default=None)

        return self._s3_temp_bucket

    @s3_temp_bucket.setter
    def s3_temp_bucket(self, key: str):
        self._s3_temp_bucket = key

    def __init__(self):
        self._s3_helper = None
        self._yc_ai_token = None
        self._azure_token = None
        self._azure_region = None
        self._s3_access_key = None
        self._s3_secret_access_key = None
        self._s3_temp_bucket = None


class S3Helper:

    def __init__(self):
        self._s3 = None

    class S3File:

        def __init__(self, owner: 'S3Helper',
                     bucket: str,
                     filename: str):
            self._owner = owner
            self._bucket = bucket
            self._filename = filename

        def url(self):
            return self._owner.url(self._bucket, self._filename)

        def delete(self):
            self._owner.delete(self._bucket, self._filename)

    def url(self, bucket, filename):
        return self._s3_client.generate_presigned_url('get_object', Params={'Bucket': bucket, 'Key': filename},
                                                      ExpiresIn=300)

    def delete(self, bucket, filename):
        return self._s3_client.delete_object(Bucket=bucket, Key=filename)

    @property
    def _s3_client(self):
        if self._s3 is None:
            self._s3 = boto3.client(
                service_name='s3',
                endpoint_url='https://storage.yandexcloud.net',
                aws_access_key_id=environment().s3_access_key,
                aws_secret_access_key=environment().s3_secret_access_key
            )
        return self._s3

    def upload_data(self, data_io):
        s3 = self._s3_client
        filename = str(uuid.uuid4())
        bucket = environment().s3_temp_bucket
        assert bucket is not None
        s3.upload_fileobj(data_io,
                          bucket,
                          filename)
        return S3Helper.S3File(owner=self,
                               bucket=bucket,
                               filename=filename)


__ENV: Environment = None
__S3_HELPER: S3Helper = None


def environment() -> Environment:
    global __ENV
    if __ENV is None:
        __ENV = Environment
    return __ENV


def s3_helper() -> S3Helper:
    global __S3_HELPER
    if __S3_HELPER is None:
        __S3_HELPER = S3Helper()
    return __S3_HELPER


def get_yandex_credentials() -> str:
    return environment().yc_ai_token


def get_azure_credentials() -> Tuple[str, str]:
    return environment().azure_token, environment().azure_region


def upload_temp_data(data_io):
    return s3_helper().upload_data(data_io)


def configure_credentials(yc_ai_token: str = None,
                          azure_token: str = None,
                          azure_region: str = None,
                          aws_access_key_id: str = None,
                          aws_secret_access_key: str = None,
                          aws_temp_bucket: str = None):
    if aws_secret_access_key is not None or aws_access_key_id is not None:
        assert aws_access_key_id is not None and aws_access_key_id is not None
    if azure_token is not None or azure_region is not None:
        assert azure_token is not None and azure_region is not None

    if yc_ai_token is not None:
        environment().yc_ai_token = yc_ai_token
    if azure_token is not None:
        environment().azure_token = azure_token
    if azure_region is not None:
        environment().azure_region = azure_region
    if aws_access_key_id is not None:
        environment().s3_access_key = aws_access_key_id
    if aws_secret_access_key is not None:
        environment().s3_secret_access_key = aws_secret_access_key
    if aws_temp_bucket is not None:
        environment().s3_temp_bucket = aws_temp_bucket
