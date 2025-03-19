from queue import Queue

from cloud.mdb.dbaas_worker.internal.providers.s3_bucket import S3Bucket
from test.mocks import _get_config

import boto3
from moto import mock_s3
import pytest
from hamcrest import assert_that, has_length, has_properties, has_entries, has_items


@pytest.fixture(scope='function')
def s3_resource(aws_credentials):
    with mock_s3():
        yield boto3.resource('s3', region_name='eu-central-1')


@pytest.fixture(scope='function')
def config():
    conf = _get_config()
    # moto require valid endpoint, it fails on "http://s3.test/foo"
    conf.s3.backup['endpoint_url'] = ''
    conf.aws.region_name = 'eu-central-1'
    conf.aws.access_key_id = 'testing-access-key'
    conf.aws.secret_access_key = 'testing-secret-key'
    return conf


def new_provider(config) -> S3Bucket:
    queue = Queue(maxsize=10000)
    task = {
        'cid': 'cid-test',
        'task_id': 'test_id',
        'task_type': 'test-task',
        'feature_flags': [],
        'folder_id': 'test_folder',
        'context': {},
        'timeout': 3600,
        'changes': [],
    }
    return S3Bucket(config, task, queue)


def test_exists__create_bucket(s3_resource, config):
    new_provider(config).exists('foo')

    all_buckets = list(s3_resource.buckets.all())
    assert_that(all_buckets, has_length(1))
    assert_that(all_buckets[0], has_properties('name', 'foo'))


def test_exists__create_bucket_with_tags_if_them_defined_in_config(s3_resource, config):
    config.aws.labels = {'foo': 'bar'}
    new_provider(config).exists('foo')

    tagging = s3_resource.BucketTagging('foo')
    assert_that(tagging.tag_set, has_length(1))
    assert_that(
        tagging.tag_set,
        has_items(
            has_entries(
                'Key',
                'foo',
                'Value',
                'bar',
            ),
        ),
    )
