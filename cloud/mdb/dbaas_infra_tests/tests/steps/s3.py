"""
Steps related to s3.
"""
import io
from tarfile import TarFile, TarInfo

import boto3
import botocore.config
from behave import given, register_type, then
from botocore.exceptions import ClientError
from hamcrest import assert_that, equal_to
from parse_type import TypeBuilder

from tests.helpers import docker
from tests.helpers.step_helpers import get_step_data
from tests.helpers.workarounds import retry

register_type(BucketExistenceVerb=TypeBuilder.make_enum({
    'has': True,
    'has no': False,
}))


@given('s3 is up and running')
@retry(wait_fixed=200, stop_max_attempt_number=25)
def wait_for_s3_alive(context):
    """
    Wait until s3 is ready to accept incoming requests.
    """
    s3_container = docker.get_container(context, 'minio01')
    _, output = s3_container.exec_run('mc admin info local')
    output = output.decode()
    if 'Uptime' not in output:
        raise RuntimeError('s3 is not available: ' + output)


@then('s3 {exists:BucketExistenceVerb} bucket for cluster')
@then('s3 {exists:BucketExistenceVerb} "{cloud_storage}" bucket for cluster')
def check_s3_bucket(context, exists, cloud_storage=None):
    """
    Check s3 bucket existence
    """
    bucket = _bucket(context)
    if cloud_storage:
        bucket = _cloud_storage_bucket(context)

    s3_container = docker.get_container(context, 'minio01')
    _, output = s3_container.exec_run('mc ls local/{bucket}'.format(bucket=bucket))
    output = output.decode()

    bucket_exists = 'ERROR' not in output
    assert_that(exists, equal_to(bucket_exists))


@given('s3 object "{object_key}"')
@given('s3 object "{object_key}" with data')
def s3_object(context, object_key):
    """
    Create empty s3 object
    """
    data = b''
    if context.text:
        data = context.text.strip().encode()
    _add_s3_object(context, object_key, data)


@given('"{object_key}" archive in s3')
def s3_object_tar(context, object_key):
    """
    Place tar archive in to s3
    """
    result = io.BytesIO()

    archive = TarFile(mode='w', fileobj=result)
    for file_name, contents in get_step_data(context).items():
        data = contents.encode()
        tarinfo = TarInfo(file_name)
        tarinfo.size = len(data)
        archive.addfile(tarinfo=tarinfo, fileobj=io.BytesIO(data))

    _add_s3_object(context, object_key, result.getvalue())


@then('s3 object absent "{object_key}"')
def s3_object_absent(context, object_key):
    """
    Ensure s3 object absent
    """
    client, bucket = _get_external_s3_client(context)
    try:
        client.head_object(Bucket=bucket, Key=object_key)
    except ClientError as err:
        assert err.response.get(
            'Error', {}).get('Code') in ('404', 'NoSuchKey'), f'failed to check s3 object {err.response.get("Error")}'
        return
    assert False, f'S3 object present {object_key}'


@then('s3 object present "{object_key}"')
@then('s3 object present "{object_key}" with data')
def s3_object_present(context, object_key):
    """
    Ensure s3 object present
    """
    client, bucket = _get_external_s3_client(context)
    data = client.get_object(Bucket=bucket, Key=object_key)['Body'].read()
    if context.text:
        assert data.decode() == context.text.strip(), \
            f'invalid data expected "{context.text.strip()}", but got "{data.decode()}"'


def _get_external_s3_client(context):
    s3_config = context.conf['dynamic']['s3']
    s3_container = docker.get_container(context, 'minio01')
    host, port = docker.get_exposed_port(s3_container, s3_config['port'])

    session = boto3.Session(
        aws_access_key_id=s3_config['access_key_id'], aws_secret_access_key=s3_config['access_secret_key'])

    endpoint_url = f'http://{host}:{port}'
    client = session.client(
        service_name='s3',
        endpoint_url=endpoint_url,
        config=botocore.config.Config(s3={
            'addressing_style': 'auto',
            'region_name': 'us-east-1',
        }))

    bucket = _bucket(context)

    return client, bucket


def _add_s3_object(context, object_key, data):
    s3_config = context.conf['dynamic']['s3']
    s3_container = docker.get_container(context, 'minio01')
    host, port = docker.get_exposed_port(s3_container, s3_config['port'])
    endpoint_url = f'http://{host}:{port}'

    client, bucket = _get_external_s3_client(context)

    client.put_object(Bucket=bucket, Key=object_key, Body=data)

    presigned_url = client.generate_presigned_url(
        ClientMethod='get_object', Params={
            'Bucket': bucket,
            'Key': object_key,
        })

    context.s3_object_url = presigned_url.replace(endpoint_url, 'http://{0}:{1}'.format(
        s3_config['host'], s3_config['port']), 1)


def _bucket(context):
    return '{prefix}{cid}'.format(prefix=context.conf['dynamic']['s3']['bucket_prefix'], cid=context.cluster['id'])


def _cloud_storage_bucket(context):
    return '{prefix}{cid}'.format(prefix='cloud-storage-', cid=context.cluster['id'])
