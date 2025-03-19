"""
Helpers for working with yandexcloud resource manager
"""

import json
import time
import boto3
import logging
from botocore.client import Config
from botocore.exceptions import ClientError
from retrying import retry

LOG = logging.getLogger('s3')


class S3Bucket:
    """
    Stub for labels property
    """
    def __init__(self, name):
        self.name = name
        self.labels = {}


def _client_create(ctx, credentials):
    """
    Setup s3 client
    """
    key_id, secret = credentials
    cfg = Config(
        connect_timeout=5.0,
        read_timeout=15.0,
        user_agent='dataproc-image-tests',
        retries={'max_attempts': 3},
    )
    endpoint = ctx.conf['s3']['endpoint']
    return boto3.client(
        's3',
        region_name=ctx.conf['s3']['region'],
        endpoint_url=f'https://{endpoint}/',
        aws_access_key_id=key_id,
        aws_secret_access_key=secret,
        config=cfg)


def client_get(ctx, credentials):
    """
    Setup s3 client in ctx if it doesn't exists
    """
    if not ctx.state.get('s3'):
        ctx.state['s3'] = _client_create(ctx, credentials)
    return ctx.state['s3']


def wait_credentials_availability(ctx, credentials, timeout=120.0):
    """
    Service-account access to s3 bucket reaches with lag, so we need to pull it before start steps
    """
    client = _client_create(ctx, credentials)
    start_at = time.time()
    while time.time() - start_at <= timeout:
        try:
            _get_all_objects(ctx, ctx.id, client=client)
            return
        except Exception as exc:
            LOG.debug('Failed attempt to check service-account access to s3 bucket', exc)
            raise
        time.sleep(5)


def bucket_list(ctx):
    """
    List all available buckets
    """
    s3 = ctx.state['s3']
    resp = s3.list_buckets()
    ret = []
    for bucket in resp['Buckets']:
        b = S3Bucket(bucket['Name'])
        try:
            resp = s3.get_object(Bucket=bucket['Name'], Key='labels.json')
            b.labels = json.loads(resp['Body'].read())
        except ClientError as ex:
            if ex.response['Error']['Code'] == 'NoSuchKey':
                # That means, that bucket doesn't have labels.
                # It's okey, just bucket not from our tests, skip it.
                pass
            else:
                raise
        finally:
            ret.append(b)
    return ret


@retry(wait_exponential_multiplier=1000, wait_exponential_max=10000, stop_max_attempt_number=5)
def bucket_create(ctx, name):
    """
    Create new bucket
    """
    s3 = ctx.state['s3']
    s3.create_bucket(Bucket=name)
    s3.put_object(Body=json.dumps(ctx.labels), Bucket=name, Key='labels.json')


def _get_all_objects(ctx, bucket, client=None):
    """
    List all objects
    """
    ret = []
    s3 = ctx.state['s3']
    if client is not None:
        s3 = client
    for page in s3.get_paginator("list_objects_v2").paginate(Bucket=bucket):
        ret.extend([obj['Key'] for obj in page['Contents']])
    return ret


def bucket_delete(ctx, name):
    """
    Delete bucket
    """
    s3 = ctx.state['s3']
    for obj in _get_all_objects(ctx, name):
        s3.delete_object(Bucket=name, Key=obj)
    s3.delete_bucket(Bucket=name)
