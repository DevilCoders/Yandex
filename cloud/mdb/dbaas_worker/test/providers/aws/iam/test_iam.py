from copy import deepcopy
from queue import Queue
import json

from cloud.mdb.dbaas_worker.internal.providers.aws.iam import AWSIAM, DEFAULT_EC2_ASSUME_ROLE_POLICY
from cloud.mdb.dbaas_worker.internal.providers.aws.iam.iam import IAMDisabledError, generate_cluster_role
from cloud.mdb.dbaas_worker.internal.types import HostGroup, GenericHost

from test.mocks import _get_config

import boto3
from moto import mock_iam, mock_sts
import pytest
from hamcrest import (
    assert_that,
    has_length,
    equal_to,
    has_items,
)


TEST_REGION = 'eu-central-1'
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
TEST_ACCOUNT_ID = '123456789012'
TEST_MANAGED_POLICY = '''{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": "s3:*",
            "Resource": "*"
        }
    ]
}'''
TEST_MANAGED_POLICY_NAME = 'test-policy'
TEST_IAM_ROLE = generate_cluster_role(TEST_TASK['cid'], TEST_ACCOUNT_ID)


@pytest.fixture(scope='function')
def sts_client(aws_credentials):
    with mock_sts():
        yield boto3.client('sts', region_name=TEST_REGION)


@pytest.fixture(scope='function')
def iam_client(sts_client, aws_credentials):
    with mock_iam():
        yield boto3.client('iam', region_name=TEST_REGION)


@pytest.fixture(scope='function')
def config(iam_client, enabled_aws_in_config):
    response = iam_client.create_policy(
        PolicyName=TEST_MANAGED_POLICY_NAME,
        PolicyDocument=TEST_MANAGED_POLICY,
    )

    enabled_aws_in_config.aws_iam.enabled = True
    enabled_aws_in_config.aws_iam.managed_dataplane_policy_arns = [response['Policy']['Arn']]
    enabled_aws_in_config.aws.dataplane_role_arn = f'arn:aws:iam::{TEST_ACCOUNT_ID}:role/Test-Role'

    return enabled_aws_in_config


def new_provider(config) -> AWSIAM:
    queue = Queue(maxsize=10000)
    task = deepcopy(TEST_TASK)
    return AWSIAM(config, task, queue)


@pytest.fixture(scope='function')
def provider_with_byoa(config, enabled_byoa) -> AWSIAM:
    return new_provider(config)


@pytest.fixture(scope='function')
def provider_without_byoa(config, disabled_byoa) -> AWSIAM:
    return new_provider(config)


def test_role_exists__for_disabled_provider():
    provider = new_provider(_get_config())
    with pytest.raises(IAMDisabledError):
        provider.role_exists(TEST_IAM_ROLE, DEFAULT_EC2_ASSUME_ROLE_POLICY, [])


def test_role_absent__for_disabled_provider():
    provider = new_provider(_get_config())
    with pytest.raises(IAMDisabledError):
        provider.role_absent(TEST_IAM_ROLE.name, [])


def assert_changes(provider: AWSIAM):
    changes = provider.task['changes']
    assert_that(changes, has_length(1))
    assert_that(changes[0].key, equal_to(f'iam.role.{TEST_IAM_ROLE.arn}'))


def assert_created_role(iam_client, config):
    role = iam_client.get_role(
        RoleName=TEST_IAM_ROLE.name,
    )['Role']
    assert_that(role['Path'], equal_to(TEST_IAM_ROLE.path))
    assert_that(role['Arn'], equal_to(TEST_IAM_ROLE.arn))
    assert_that(role['AssumeRolePolicyDocument'], equal_to(json.loads(DEFAULT_EC2_ASSUME_ROLE_POLICY)))

    policies = iam_client.list_attached_role_policies(
        RoleName=TEST_IAM_ROLE.name,
    )['AttachedPolicies']
    assert_that(
        [x['PolicyArn'] for x in policies],
        has_items(*[equal_to(arn) for arn in config.aws_iam.managed_dataplane_policy_arns]),
    )

    profile = iam_client.get_instance_profile(
        InstanceProfileName=TEST_IAM_ROLE.name,
    )['InstanceProfile']
    assert_that(profile['Path'], equal_to(TEST_IAM_ROLE.path))
    assert_that(profile['Arn'], equal_to(TEST_IAM_ROLE.instance_profile_arn))
    assert_that(profile['Roles'], has_length(1))
    assert_that(profile['Roles'][0]['Path'], equal_to(TEST_IAM_ROLE.path))
    assert_that(profile['Roles'][0]['RoleName'], equal_to(TEST_IAM_ROLE.name))


def assert_inline_policy_byoa(iam_client):
    policies = iam_client.list_role_policies(RoleName=TEST_IAM_ROLE.name)
    assert_that(policies['PolicyNames'], has_length(1))
    assert_that(policies['PolicyNames'][0], equal_to('DoubleCloudPolicy'))


def assert_inline_policy_wo_byoa(iam_client):
    policies = iam_client.list_role_policies(RoleName=TEST_IAM_ROLE.name)
    assert_that(policies['PolicyNames'], has_length(0))


def test_role_exists__with_byoa(config, provider_with_byoa: AWSIAM, iam_client):
    # even when we add BYOA pillars, moto returns caller identity in default account(
    # ipdb> sts_cl.get_caller_identity()
    # {'Account': '123456789012', 'Arn': 'arn:aws:sts::123456789012:assumed-role/BYOA/botocore-session-1658311726'}
    # so we can not check that IAM provider creates different roles in different accounts with the same name =(

    provider_with_byoa.role_exists(
        TEST_IAM_ROLE,
        DEFAULT_EC2_ASSUME_ROLE_POLICY,
        config.aws_iam.managed_dataplane_policy_arns,
    )
    assert_changes(provider_with_byoa)
    assert_created_role(iam_client, config)
    assert_inline_policy_byoa(iam_client)


def test_role_exists__without_byoa(config, provider_without_byoa: AWSIAM, iam_client):
    provider_without_byoa.role_exists(
        TEST_IAM_ROLE,
        DEFAULT_EC2_ASSUME_ROLE_POLICY,
        config.aws_iam.managed_dataplane_policy_arns,
    )
    assert_changes(provider_without_byoa)
    assert_created_role(iam_client, config)
    assert_inline_policy_wo_byoa(iam_client)


def test_role_absent__with_byoa(config, provider_with_byoa: AWSIAM, iam_client):
    provider_with_byoa.role_absent(
        TEST_IAM_ROLE.name,
        config.aws_iam.managed_dataplane_policy_arns,
    )
