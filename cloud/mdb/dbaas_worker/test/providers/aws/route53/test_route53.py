from queue import Queue
import uuid

from cloud.mdb.dbaas_worker.internal.providers.dns import Record
from cloud.mdb.dbaas_worker.internal.providers.aws.route53.route53 import Route53, Route53DisabledError

from test.mocks import _get_config

import boto3
from moto import mock_route53
import pytest
from hamcrest import assert_that, has_length, has_entries, has_item, has_items, empty

TEST_REGION = 'eu-central-1'


@pytest.fixture(scope='function')
def route53_client(aws_credentials):
    with mock_route53():
        yield boto3.client('route53', region_name='eu-central-1')


@pytest.fixture(scope='function')
def config(route53_client, enabled_aws_in_config):
    enabled_aws_in_config.route53.enabled = True
    response = route53_client.create_hosted_zone(
        Name='dc.eu',
        CallerReference=str(uuid.uuid4()),
    )
    enabled_aws_in_config.route53.public_hosted_zone_id = response['HostedZone']['Id']
    # That zone should be `private` (linked to a VPC),
    # but the worker doesn't use that fact.
    # So I simplify the tests setup.
    response = route53_client.create_hosted_zone(
        Name='dc.eu',
        CallerReference=str(uuid.uuid4()),
    )
    enabled_aws_in_config.route53.private_hosted_zones_ids = {TEST_REGION: response['HostedZone']['Id']}
    return enabled_aws_in_config


def new_provider(config) -> Route53:
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
    return Route53(config, task, queue)


@pytest.fixture(scope='function')
def provider(config, enabled_byoa) -> Route53:
    return new_provider(config)


def test_set_records_in_public_zone__for_disabled_provider():
    provider = new_provider(_get_config())
    with pytest.raises(Route53DisabledError):
        provider.set_records_in_public_zone('test.db.eu', [])


def has_resource_record(fqdn, record):
    """
    Return matcher that match `ResourceRecordSets` part of  `list_resource_record_sets` response
    for given fqdn and record
    https://nda.ya.ru/t/CJb6FtYS42Grb6
    """
    return has_entries(
        'Name',
        fqdn,
        'Type',
        record.record_type,
        'ResourceRecords',
        has_length(1),
        'ResourceRecords',
        has_item(
            has_entries(
                'Value',
                record.address,
            ),
        ),
    )


def assert_hosted_zone_has_one_record(hosted_zone_id, route53_client, fqdn, record):
    response = route53_client.list_resource_record_sets(HostedZoneId=hosted_zone_id)
    assert_that(response['ResourceRecordSets'], has_length(1))
    assert_that(
        response['ResourceRecordSets'],
        has_item(has_resource_record(fqdn, record)),
    )


def test_set_records_in_public_zone__add_record(route53_client, provider):
    provider.set_records_in_public_zone('test.dc.eu', [Record('10.10.10.10', 'A')])
    assert_hosted_zone_has_one_record(
        provider.config.route53.public_hosted_zone_id, route53_client, 'test.dc.eu.', Record('10.10.10.10', 'A')
    )


def test_set_records_in_public_zone__for_more_then_one_record(route53_client, provider):
    provider.set_records_in_public_zone('test.dc.eu', [Record('10.10.10.10', 'A'), Record('11.11.11.11', 'A')])
    response = route53_client.list_resource_record_sets(HostedZoneId=provider.config.route53.public_hosted_zone_id)
    assert_that(
        response['ResourceRecordSets'],
        has_items(
            has_resource_record('test.dc.eu.', Record('10.10.10.10', 'A')),
            has_resource_record('test.dc.eu.', Record('11.11.11.11', 'A')),
        ),
    )


def test_set_records_in_public_zone__add_record__idempotency(route53_client, provider):
    provider.set_records_in_public_zone('test.dc.eu', [Record('10.10.10.10', 'A')])
    provider.set_records_in_public_zone('test.dc.eu', [Record('10.10.10.10', 'A')])
    assert_hosted_zone_has_one_record(
        provider.config.route53.public_hosted_zone_id, route53_client, 'test.dc.eu.', Record('10.10.10.10', 'A')
    )


def assert_hosted_zone_is_empty(hosted_zone_id, route53_client):
    response = route53_client.list_resource_record_sets(
        HostedZoneId=hosted_zone_id,
    )
    assert_that(response['ResourceRecordSets'], empty())


def test_set_records_in_public_zone__empty_records__delete_records(route53_client, provider):
    provider.set_records_in_public_zone('test.dc.eu', [Record('10.10.10.10', 'A')])
    provider.set_records_in_public_zone('test.dc.eu', [])
    assert_hosted_zone_is_empty(provider.config.route53.public_hosted_zone_id, route53_client)


def test_set_records_in_public_zone__switch_records(route53_client, provider):
    provider.set_records_in_public_zone('test.dc.eu', [Record('10.10.10.10', 'A')])
    provider.set_records_in_public_zone('test.dc.eu', [Record('11.11.11.11', 'A')])
    assert_hosted_zone_has_one_record(
        provider.config.route53.public_hosted_zone_id, route53_client, 'test.dc.eu.', Record('11.11.11.11', 'A')
    )


def test_set_records_in_public_zone__create_rollback(route53_client, provider):
    provider.set_records_in_public_zone('test.dc.eu', [Record('10.10.10.10', 'A')])
    for change in provider.task['changes']:
        change.rollback(provider.task, 42)
    assert_hosted_zone_is_empty(provider.config.route53.public_hosted_zone_id, route53_client)


def test_set_records_in_public_zone__delete_rollback(route53_client, provider):
    provider.set_records_in_public_zone('test.dc.eu', [Record('10.10.10.10', 'A')])
    another_provider = new_provider(provider.config)
    another_provider.set_records_in_public_zone('test.dc.eu', [])
    for change in another_provider.task['changes']:
        change.rollback(provider.task, 42)
    assert_hosted_zone_has_one_record(
        provider.config.route53.public_hosted_zone_id, route53_client, 'test.dc.eu.', Record('10.10.10.10', 'A')
    )


def test_set_records_in_public_zone__switch_records_rollback(route53_client, provider):
    provider.set_records_in_public_zone('test.dc.eu', [Record('10.10.10.10', 'A')])
    another_provider = new_provider(provider.config)
    another_provider.set_records_in_public_zone('test.dc.eu', [Record('11.11.11.11', 'A')])
    # Rollback changes in order cause:
    #
    # In rollback we do:
    # - [{'Action': 'CREATE', 'ResourceRecordSet': {
    #       'Name': 'test.dc.eu.', 'Type': 'A', 'TTL': 300,
    #       'ResourceRecords': [{'Value': '10.10.10.10'}]}}]
    # - [{'Action': 'DELETE', 'ResourceRecordSet': {
    #       'Name': 'test.dc.eu.', 'Type': 'A', 'TTL': 300,
    #       'ResourceRecords': [{'Value': '11.11.11.11'}]}}]
    #
    # And there are no records in hosted zone 🤦
    # Probably it 'implements' Route 53 a little bit simple. (One record per FQDN?)
    for change in reversed(another_provider.task['changes']):
        change.rollback(provider.task, 42)
    assert_hosted_zone_has_one_record(
        provider.config.route53.public_hosted_zone_id, route53_client, 'test.dc.eu.', Record('10.10.10.10', 'A')
    )
