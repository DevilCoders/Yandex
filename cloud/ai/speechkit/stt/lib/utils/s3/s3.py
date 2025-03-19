import os
import typing

import boto3
from botocore.config import Config

from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts


def create_client(
    endpoint_url: str = s3_consts.cloud_url,
    access_key_id: typing.Optional[str] = None,
    secret_access_key: typing.Optional[str] = None,
):
    access_key_id = access_key_id or os.getenv('AWS_ACCESS_KEY_ID')
    secret_access_key = secret_access_key or os.getenv('AWS_SECRET_ACCESS_KEY')
    return boto3.session.Session().client(
        service_name='s3',
        endpoint_url=endpoint_url,
        aws_access_key_id=access_key_id,
        aws_secret_access_key=secret_access_key,
        config=Config(retries=dict(max_attempts=30)),
    )
