"""
Utilities for working with Object Storage
"""
from io import BytesIO

from boto3.session import Session
from botocore.exceptions import ConnectionError as BotocoreConnectionError
from botocore.exceptions import HTTPClientError
from retrying import retry

from cloud.mdb.infratests.test_helpers.context import Context


def combined_content(context: Context, bucket_name: str, folder: str):
    """
    List objects from bucket in one string
    """
    objects = list_objects(context, bucket_name, folder)
    return ''.join([_content_as_string(obj) for obj in objects])


def download_object(context: Context, bucket_name: str, object_name: str) -> str:
    """
    Download s3 object
    """
    client = _make_resource(context)
    obj = client.Object(bucket_name, object_name)  # pylint: disable=no-member
    return _content_as_string(obj)


@retry(
    stop_max_attempt_number=5,
    retry_on_exception=lambda exc: isinstance(exc, (BotocoreConnectionError, HTTPClientError)),
)
def list_objects(context: Context, bucket_name: str, folder: str):
    """
    List S3 objects from bucket
    """
    client = _make_resource(context)
    bucket = client.Bucket(name=bucket_name)  # pylint: disable=no-member
    return bucket.objects.filter(Prefix=folder)


@retry(
    stop_max_attempt_number=5,
    retry_on_exception=lambda exc: isinstance(exc, (BotocoreConnectionError, HTTPClientError)),
)
def _content_as_string(obj):
    return BytesIO(obj.get()['Body'].read()).read().decode('utf-8')


def _make_resource(context: Context):
    config = context.test_config
    session = Session(
        aws_access_key_id=config.user_service_account.s3_access_key,
        aws_secret_access_key=config.user_service_account.s3_secret_key,
    )
    return session.resource('s3', endpoint_url=f'https://{config.s3.endpoint_url}', region_name=config.s3.region_name)
