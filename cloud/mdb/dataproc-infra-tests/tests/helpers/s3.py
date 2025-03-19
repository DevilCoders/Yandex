#!/usr/bin/env python3
"""
Some s3 helpers
"""
import os
from io import BytesIO

from boto3.session import Session
from botocore.exceptions import ConnectionError as BotocoreConnectionError
from botocore.exceptions import HTTPClientError
from retrying import retry


def _make_resource(s3config):
    session = Session(aws_access_key_id=s3config['access_key'], aws_secret_access_key=s3config['secret_key'])
    return session.resource(
        's3', endpoint_url=f'https://{s3config["endpoint_url"]}', region_name=s3config['region_name']
    )


def _make_client(s3config):
    session = Session(aws_access_key_id=s3config['access_key'], aws_secret_access_key=s3config['secret_key'])
    return session.client('s3', endpoint_url=f'https://{s3config["endpoint_url"]}', region_name=s3config['region_name'])


def download_object(s3config, bucket_name, object_name):
    """
    Download s3 object
    """
    client = _make_resource(s3config)
    obj = client.Object(bucket_name, object_name)  # pylint: disable=no-member
    return _content_as_string(obj)


@retry(
    stop_max_attempt_number=5,
    retry_on_exception=lambda exc: isinstance(exc, (BotocoreConnectionError, HTTPClientError)),
)
def _content_as_string(obj):
    return BytesIO(obj.get()['Body'].read()).read().decode('utf-8')


@retry(
    stop_max_attempt_number=5,
    retry_on_exception=lambda exc: isinstance(exc, (BotocoreConnectionError, HTTPClientError)),
)
def put_object(s3config, bucket_name, src, dst):
    """
    Put file from filesystem to object storage
    """
    client = _make_resource(s3config)
    return client.Bucket(bucket_name).upload_file(os.path.abspath(src), dst, ExtraArgs={'ACL': 'public-read'})


@retry(
    stop_max_attempt_number=5,
    retry_on_exception=lambda exc: isinstance(exc, (BotocoreConnectionError, HTTPClientError)),
)
def list_objects(s3config, bucket_name, folder):
    """
    List S3 objects from bucket
    """
    client = _make_resource(s3config)
    bucket = client.Bucket(name=bucket_name)  # pylint: disable=no-member
    return bucket.objects.filter(Prefix=folder)


def combined_content(s3config, bucket_name, folder):
    """
    List objects from bucket in one string
    """
    objects = list_objects(s3config, bucket_name, folder)
    return ''.join([_content_as_string(obj) for obj in objects])


@retry(
    stop_max_attempt_number=5,
    retry_on_exception=lambda exc: isinstance(exc, (BotocoreConnectionError, HTTPClientError)),
)
def list_buckets(s3config):
    """
    List S3 buckets
    """
    client = _make_client(s3config)
    response = client.list_buckets()  # pylint: disable=no-member
    return response.get('Buckets', []), response.get('Owner', {})


@retry(
    stop_max_attempt_number=5,
    retry_on_exception=lambda exc: isinstance(exc, (BotocoreConnectionError, HTTPClientError)),
)
def create_bucket(s3config, bucket_name):
    bucket = _make_resource(s3config).Bucket(bucket_name)
    if not bucket.creation_date:
        bucket.create()


@retry(
    stop_max_attempt_number=5,
    retry_on_exception=lambda exc: isinstance(exc, (BotocoreConnectionError, HTTPClientError)),
)
def delete_bucket(s3config, bucket_name):
    """
    Deletes S3 bucket (and all content)
    """
    bucket = _make_resource(s3config).Bucket(bucket_name)
    bucket.objects.all().delete()
    bucket.delete()
