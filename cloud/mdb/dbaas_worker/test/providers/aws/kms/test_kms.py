from queue import Queue

from cloud.mdb.dbaas_worker.internal.providers.aws.kms.kms import KMS, KMSDisabledError
from cloud.mdb.dbaas_worker.internal.tasks.utils import get_cluster_encryption_key_alias
from cloud.mdb.dbaas_worker.internal.types import HostGroup, GenericHost

from test.mocks import _get_config

import boto3
from moto import mock_kms, mock_sts
import pytest


TEST_REGION = 'eu-central-1'
CONTEXT_KEY = 'kms.key'
TEST_TASK = {
    'cid': 'cid-test',
    'task_id': 'test_id',
    'task_type': 'test-task',
    'feature_flags': [],
    'folder_id': 'test_folder',
    'context': {},
    'timeout': 3600,
    'changes': [],
}
TEST_HOST_GROUP = HostGroup(None, {'host1': GenericHost(**{'vtype': 'aws'})})
TEST_KEY_ALIAS = get_cluster_encryption_key_alias(TEST_HOST_GROUP, TEST_TASK['cid'])
TEST_ROLE_ARN = 'arn:aws:iam::123456789012:role/Test-Role'


@pytest.fixture(scope='function')
def sts_client(aws_credentials):
    with mock_sts():
        yield boto3.client('sts', region_name=TEST_REGION)


@pytest.fixture(scope='function')
def kms_client(sts_client, aws_credentials):
    with mock_kms():
        yield boto3.client('kms', region_name=TEST_REGION)


@pytest.fixture(scope='function')
def config(enabled_aws_in_config):
    enabled_aws_in_config.kms.enabled = True
    enabled_aws_in_config.aws.dataplane_role_arn = TEST_ROLE_ARN
    return enabled_aws_in_config


def new_provider(config) -> KMS:
    queue = Queue(maxsize=10000)
    return KMS(config, TEST_TASK, queue)


@pytest.fixture(scope='function')
def provider(config, enabled_byoa) -> KMS:
    return new_provider(config)


def test_create_key__for_disabled_provider():
    provider = new_provider(_get_config())
    with pytest.raises(KMSDisabledError):
        provider.key_exists(TEST_KEY_ALIAS, TEST_ROLE_ARN, TEST_REGION)


def test_absent_key__for_disabled_provider():
    provider = new_provider(_get_config())
    with pytest.raises(KMSDisabledError):
        provider.key_absent(TEST_KEY_ALIAS, TEST_REGION)


# NotImplementedError: The create_grant action has not been implemented
# TODO: Need to update moto contrib.
# def test_create_key(provider, kms_client):
#     provider.key_exists(TEST_KEY_ALIAS, TEST_ROLE_ARN)
#     key_id = provider.context_get(CONTEXT_KEY).get('id')
#
#     key = kms_client.describe_key(KeyId=TEST_KEY_ALIAS)
#     assert_that(key['KeyMetadata']['KeyId'], equal_to(key_id))
