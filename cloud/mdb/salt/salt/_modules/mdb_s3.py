# -*- coding: utf-8 -*-
"""
Module for managing S3 objects in MDB.
"""
import logging
import re
from functools import wraps

try:
    from boto3 import Session
    from botocore.config import Config
    from botocore.exceptions import ClientError

    HAS_BOTO = True
except:
    HAS_BOTO = False

log = logging.getLogger(__name__)

__salt__ = {}


def __virtual__():
    return True


def _log_s3_errors(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except ClientError as e:
            log.error('Received S3 error: %s', e.response)
            raise

    return wrapper


def bucket():
    """
    Return cluster bucket.
    """
    return __salt__['pillar.get']('data:s3_bucket')


def endpoint(protocol=None):
    """
    Return S3 endpoint.
    """
    pillar_value = __salt__['pillar.get']('data:s3:endpoint')
    if not pillar_value:
        return None

    if protocol is None:
        return pillar_value.replace('+path://', '://')

    return re.sub(r'^https?(\+path)?://', '{}://'.format(protocol), pillar_value)


def client():
    """
    Create and return S3 client.
    """
    assert HAS_BOTO, 'boto modules not available'

    s3_pillar = __salt__['pillar.get']('data:s3', {})

    session = Session(
        aws_access_key_id=s3_pillar.get('access_key_id'), aws_secret_access_key=s3_pillar.get('access_secret_key')
    )

    endpoint_url = s3_pillar.get('endpoint')
    if endpoint_url:
        endpoint_url = endpoint_url.replace('+path://', '://')

    s3_config = {}
    if s3_pillar.get('virtual_addressing_style'):
        s3_config['addressing_style'] = 'virtual'
    else:
        s3_config['addressing_style'] = 'path'
    if s3_pillar.get('region'):
        s3_config['region_name'] = s3_pillar['region']

    return session.client(service_name='s3', endpoint_url=endpoint_url, config=Config(s3=s3_config))


@_log_s3_errors
def object_exists(s3_client, key, s3_bucket=None):
    """
    Return `True` if the object exists.
    """
    try:
        s3_client.head_object(Bucket=s3_bucket or bucket(), Key=key)
        return True
    except ClientError as e:
        if _error_code(e) not in ('404', 'NoSuchKey'):
            raise

    return False


@_log_s3_errors
def object_absent(s3_client, key, s3_bucket=None):
    """
    Deletes object in s3.
    """
    try:
        s3_client.delete_object(Bucket=s3_bucket or bucket(), Key=key)
    except ClientError as e:
        if _error_code(e) not in ('404', 'NoSuchKey'):
            raise


@_log_s3_errors
def create_object(s3_client, key, data=b'', s3_bucket=None):
    """
    Create a new object.
    """
    s3_client.put_object(Bucket=s3_bucket or bucket(), Key=key, Body=data)


@_log_s3_errors
def get_object(s3_client, key, s3_bucket=None):
    """
    Read existing object.
    """
    return s3_client.get_object(Bucket=s3_bucket or bucket(), Key=key)['Body'].read()


def _error_code(exception):
    return exception.response.get('Error', {}).get('Code')
