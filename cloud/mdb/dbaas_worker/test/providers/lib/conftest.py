from copy import deepcopy
import os

import pytest

from test.mocks import _get_config, DEFAULT_STATE
from test.mocks.metadb import metadb


@pytest.fixture(scope='function')
def aws_credentials():
    """Mocked AWS Credentials for moto."""
    os.environ['AWS_ACCESS_KEY_ID'] = 'testing-access-key'
    os.environ['AWS_SECRET_ACCESS_KEY'] = 'testing-secret-key'
    os.environ['AWS_SECURITY_TOKEN'] = 'testing'
    os.environ['AWS_SESSION_TOKEN'] = 'testing'


@pytest.fixture(scope='function')
def enabled_aws_in_config():
    config = _get_config()
    # Sadly, but we can't use 'test-region' or something like that, cause boto fails on it
    config.aws.region_name = 'eu-central-1'
    config.aws.access_key_id = 'testing-access-key'
    config.aws.secret_access_key = 'testing-secret-key'
    config.aws.labels = {'control-plane': 'ya-tests'}
    return config


@pytest.fixture(scope='function')
def enabled_byoa(mocker):
    state = deepcopy(DEFAULT_STATE)
    state['metadb']['queries'].insert(
        0,
        {
            'query': 'get_pillar',
            'kwargs': {'path': ['data', 'byoa']},
            'result': [{'value': {'iam_role_arn': 'arn:aws:iam::00000000:role/BYOA'}}],
        },
    )
    metadb(mocker, state)


@pytest.fixture(scope='function')
def disabled_byoa(mocker):
    metadb(mocker, DEFAULT_STATE)
