from queue import Queue
import logging

from cloud.mdb.dbaas_worker.internal.providers.aws.ec2 import (
    EC2,
    EC2InstanceSpec,
    EC2DiscSpec,
    EC2APIError,
    EC2InstanceChange,
)
from cloud.mdb.dbaas_worker.internal.providers.aws.ec2.instance import EC2InstanceAddresses
from cloud.mdb.dbaas_worker.internal.providers.aws.ec2.ec2 import UnsupportedError
from cloud.mdb.dbaas_worker.internal.providers.dns import Record

import boto3
from moto import mock_ec2, mock_sts
import pytest
from hamcrest import (
    assert_that,
    has_length,
    has_properties,
    has_entries,
    has_items,
    described_as,
    empty,
    equal_to,
)


TEST_REGION = 'eu-central-1'
TEST_AZ = 'eu-central-1a'
TEST_IAM_PROFILE = 'arn:aws:iam::123456789012:role/Instance-Profile'
TEST_INSTANCES_SPEC = EC2InstanceSpec(
    name='test.dc.eu',
    boot=EC2DiscSpec(
        type='gp2',
        size=20 * 2**30,
    ),
    data=EC2DiscSpec(
        type='standard',
        size=100 * 2**30,
    ),
    instance_type='x1.test',
    image_type='test',
    labels={'cluster_id': 'test-cid'},
    userdata='some-user-data',
    subnet_id='',
    sg_id='',
    iam_instance_profile=TEST_IAM_PROFILE,
)

log = logging.getLogger(__name__)


@pytest.fixture(scope='function')
def ec2_resource(aws_credentials):
    with mock_ec2():
        yield boto3.resource('ec2', region_name=TEST_REGION)


@pytest.fixture(scope='function')
def sts_client(aws_credentials):
    with mock_sts():
        yield boto3.client('sts', region_name=TEST_REGION)


@pytest.fixture(scope='function')
def config(ec2_resource, enabled_aws_in_config):
    """
    Create
    - AMI
    - Public VPC
    - Private VPC
    """
    enabled_aws_in_config.ec2.enabled = True
    vm4img = ec2_resource.create_instances(
        ImageId='test-vm4img',
        MinCount=1,
        MaxCount=1,
    )[0]
    vm4img.create_image(Name='dc-aws-test-1999-09-09T20:20:20')
    vm4img.terminate()

    vpc = ec2_resource.create_vpc(
        CidrBlock='10.0.0.0/16',
        Ipv6Pool='2a05:d014:419:1700::/56',
    )
    subnet = vpc.create_subnet(
        CidrBlock='10.0.10.0/16',
        AvailabilityZone=TEST_AZ,
        Ipv6CidrBlock='2a05:d014:419:1700::/64',
    )
    sg = ec2_resource.create_security_group(
        GroupName='SECURITY_GROUP_NAME',
        Description='DESCRIPTION',
        VpcId=vpc.id,
    )

    global TEST_INSTANCES_SPEC
    spec = EC2InstanceSpec(
        boot=TEST_INSTANCES_SPEC.boot,
        data=TEST_INSTANCES_SPEC.data,
        instance_type=TEST_INSTANCES_SPEC.instance_type,
        image_type=TEST_INSTANCES_SPEC.image_type,
        name=TEST_INSTANCES_SPEC.name,
        labels=TEST_INSTANCES_SPEC.labels,
        userdata=TEST_INSTANCES_SPEC.userdata,
        sg_id=sg.id,
        subnet_id=subnet.id,
        iam_instance_profile=TEST_IAM_PROFILE,
    )
    TEST_INSTANCES_SPEC = spec

    # adding standard disk type, cause looks like that moto doesn't support
    # other disk type in `run_instances`
    enabled_aws_in_config.ec2.supported_disk_types = ['standard', 'gp2', 'gp3']

    enabled_aws_in_config.aws.dataplane_role_arn = 'arn:aws:iam::123456789012:role/Test-Role'
    return enabled_aws_in_config


def new_provider(provider_config) -> EC2:
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
    return EC2(provider_config, task, queue)


@pytest.fixture(scope='function')
def provider(config, enabled_byoa) -> EC2:
    return new_provider(config)


def exists_one_running_instance(ec2_resource, description='one running instance'):
    instaces = list(ec2_resource.instances.filter(Filters=[{'Name': 'instance-state-name', 'Values': ['running']}]))
    assert_that(instaces, described_as(description, has_length(1)))
    return instaces[0]


def exists_one_stopped_instance(ec2_resource, description='one stopped instance'):
    instaces = list(ec2_resource.instances.filter(Filters=[{'Name': 'instance-state-name', 'Values': ['stopped']}]))
    assert_that(instaces, described_as(description, has_length(1)))
    return instaces[0]


def test_instance_exists__create_instance(ec2_resource, sts_client, provider):
    provider.instance_exists(
        EC2InstanceSpec(
            name='test.dc.eu',
            boot=EC2DiscSpec(
                type='gp2',
                size=30 * 2**30,
            ),
            data=EC2DiscSpec(
                type='gp3',
                size=42 * 2**30,
            ),
            instance_type='x1.test',
            image_type='test',
            labels={'cluster_id': 'test-cid'},
            userdata='some-user-data',
            subnet_id=TEST_INSTANCES_SPEC.subnet_id,
            sg_id=TEST_INSTANCES_SPEC.sg_id,
            iam_instance_profile=TEST_IAM_PROFILE,
        ),
        TEST_REGION,
    )
    instance = exists_one_running_instance(ec2_resource)
    assert_that(
        instance,
        has_properties(
            'instance_type',
            'x1.test',
            'tags',
            has_items(
                has_entries(
                    'Key',
                    'Name',
                    'Value',
                    'test.dc.eu',
                ),
                has_entries(
                    'Key',
                    'cluster_id',
                    'Value',
                    'test-cid',
                ),
                has_entries(
                    'Key',
                    'control-plane',
                    'Value',
                    'ya-tests',
                ),
            ),
            'network_interfaces',
            has_length(1),
            'state',
            has_entries(
                'Name',
                'running',
            ),
        ),
    )
    # Sadly, looks a like that moto doesn't implement block devices.
    # TODO: Upgrade moto and write that checks
    # Created VM has one device with size 8GiB


def test_instance_exists__when__instance_already_exists(ec2_resource, sts_client, provider):
    _ = ec2_resource.create_instances(
        MinCount=1,
        MaxCount=1,
        TagSpecifications=[{'Tags': [{'Key': 'Name', 'Value': TEST_INSTANCES_SPEC.name}], 'ResourceType': 'instance'}],
    )[0]
    exists_one_running_instance(ec2_resource)
    provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
    exists_one_running_instance(ec2_resource)


def test_instance_exists__for__malformed_disk_size(ec2_resource, sts_client, provider):
    with pytest.raises(EC2APIError):
        provider.instance_exists(
            EC2InstanceSpec(
                name='test.dc.eu',
                boot=EC2DiscSpec(
                    type='gp2',
                    size=11111,
                ),
                data=EC2DiscSpec(
                    type='gp3',
                    size=33333,
                ),
                instance_type='x1.test',
                image_type='test',
                labels={'cluster_id': 'test-cid'},
                userdata='some-user-data',
                subnet_id=TEST_INSTANCES_SPEC.subnet_id,
                sg_id=TEST_INSTANCES_SPEC.sg_id,
                iam_instance_profile=TEST_IAM_PROFILE,
            ),
            TEST_REGION,
        )


def test_instance_wait_until_running__for__running_instance(ec2_resource, sts_client, provider):
    instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
    provider.instance_wait_until_running(instance_id, TEST_REGION)
    exists_one_running_instance(ec2_resource)


def test_instance_wait_until_running__for__stopped_instance(ec2_resource, sts_client, provider):
    instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
    list(ec2_resource.instances.filter(InstanceIds=[instance_id]))[0].stop()
    provider.instance_wait_until_running(instance_id, TEST_REGION)
    exists_one_running_instance(ec2_resource)


def assert_instance_is_terminated(ec2_resource, instance_id):
    instaces = list(ec2_resource.instances.filter(InstanceIds=[instance_id]))
    assert_that(instaces, has_length(1))
    assert_that(
        instaces[0],
        has_properties(
            'state',
            has_entries(
                'Name',
                'terminated',
            ),
        ),
    )


def test_instance_absent__for_running_instance(ec2_resource, sts_client, provider):
    instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
    provider.instance_absent(TEST_INSTANCES_SPEC.name, TEST_REGION)

    assert_instance_is_terminated(ec2_resource, instance_id)


def test_instance_absent__for_terminated(ec2_resource, sts_client, provider):
    provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
    provider.instance_absent(TEST_INSTANCES_SPEC.name, TEST_REGION)

    assert_that(provider.instance_absent(TEST_INSTANCES_SPEC.name, TEST_REGION), empty())


def test_instance_wait_until_terminated(ec2_resource, sts_client, provider):
    instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
    provider.instance_absent(TEST_INSTANCES_SPEC.name, TEST_REGION)
    provider.instance_wait_until_terminated(instance_id, TEST_REGION)

    assert_instance_is_terminated(ec2_resource, instance_id)


def test_instance_wait_until_terminatedt__for_not_existed(ec2_resource, sts_client, provider):
    instance_id = 'i-notexistedinstace'
    provider.instance_wait_until_terminated(instance_id, TEST_REGION)


def test_get_instance_addresses_by_name(ec2_resource, sts_client, provider):
    instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
    try:
        # Looks a like that my world setup incomplete,
        # cause moto doesn't assign V6 address to VMs.
        # TODO: test it with updated moto
        addresses = provider.get_instance_addresses(instance_id, TEST_REGION)
        assert addresses.public_v4 is not None
    except EC2APIError:
        pass


def _create_instance_with_data_disk(provider) -> str:
    instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
    # Sadly, looks a like that it was a bug (or not implemented behaviour) in moto
    # TODO: Upgrade moto and write that checks
    # Created VM has only one device with size 8GiB
    ec2_client = boto3.client('ec2', region_name=TEST_REGION)
    response = ec2_client.create_volume(
        AvailabilityZone=TEST_AZ,
        Size=TEST_INSTANCES_SPEC.data.size // 2**30,
        VolumeType=TEST_INSTANCES_SPEC.data.type,
    )
    volume_id = response['VolumeId']
    ec2_client.attach_volume(
        Device='/dev/sdf',
        InstanceId=instance_id,
        VolumeId=volume_id,
    )
    return instance_id


class WithMockedSTS:
    """
    We don't use STS directly in tests but should mock it cause EC2 provider use it
    """

    sts_mocker = mock_sts()

    def setup_method(self, method):
        self.sts_mocker.start()

    def teardown_method(self, method):
        self.sts_mocker.stop()


class Test_get_instance_changes(WithMockedSTS):  # noqa
    def test_unchanged_instance(self, provider):
        instance_id = _create_instance_with_data_disk(provider)
        changes = provider.get_instance_changes(
            instance_id, TEST_INSTANCES_SPEC.data, TEST_INSTANCES_SPEC.instance_type, TEST_REGION
        )
        assert_that(changes, empty())

    def test_disk_scale_up(self, provider):
        instance_id = _create_instance_with_data_disk(provider)
        bigger_disk = EC2DiscSpec(
            type=TEST_INSTANCES_SPEC.data.type,
            size=TEST_INSTANCES_SPEC.data.size + 100 * 2**30,
        )
        changes = provider.get_instance_changes(
            instance_id, bigger_disk, TEST_INSTANCES_SPEC.instance_type, TEST_REGION
        )
        assert_that(changes, equal_to({EC2InstanceChange.disk_size_up}))

    def test_disk_scale_down_not_supported(self, provider):
        instance_id = _create_instance_with_data_disk(provider)
        small_disk = EC2DiscSpec(
            type=TEST_INSTANCES_SPEC.data.type,
            size=2**30,
        )
        with pytest.raises(UnsupportedError):
            provider.get_instance_changes(instance_id, small_disk, TEST_INSTANCES_SPEC.instance_type, TEST_REGION)

    def test_disk_type_not_supported(self, provider):
        instance_id = _create_instance_with_data_disk(provider)
        data_disk = EC2DiscSpec(
            type="gp3",
            size=TEST_INSTANCES_SPEC.data.size,
        )
        with pytest.raises(UnsupportedError):
            provider.get_instance_changes(instance_id, data_disk, TEST_INSTANCES_SPEC.instance_type, TEST_REGION)

    def test_instance_type(self, provider):
        instance_id = _create_instance_with_data_disk(provider)
        changes = provider.get_instance_changes(instance_id, TEST_INSTANCES_SPEC.data, 'y2.test', TEST_REGION)
        assert_that(changes, equal_to({EC2InstanceChange.instance_type}))


class Test_set_instance_data_disk_size(WithMockedSTS):  # noqa
    def test_set_to_same_size(self, provider):
        instance_id = _create_instance_with_data_disk(provider)
        provider.set_instance_data_disk_size(instance_id, TEST_INSTANCES_SPEC.data.size, TEST_REGION)

    # There are no other cases for disk modify,
    # cause volume modification not implemented in moto.
    # It fails with:
    #
    #   NotImplementedError: The modify_volume action has not been implemented
    #


class Test_instance_stopped(WithMockedSTS):  # noqa
    def test_for_running(self, provider, ec2_resource):
        instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
        provider.instance_stopped(instance_id, TEST_REGION)
        exists_one_stopped_instance(ec2_resource)

    def test_for_stopped(self, provider, ec2_resource):
        instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
        ec2_resource.Instance(instance_id).stop()
        provider.instance_stopped(instance_id, TEST_REGION)
        exists_one_stopped_instance(ec2_resource)

    def test_rollback(self, provider, ec2_resource):
        instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)

        another_provider = new_provider(provider.config)
        another_provider.instance_stopped(instance_id, TEST_REGION)
        for change in reversed(another_provider.task['changes']):
            change.rollback(provider.task, 42)

        exists_one_running_instance(ec2_resource)


class Test_instance_running(WithMockedSTS):  # noqa
    def test_for_running(self, provider, ec2_resource):
        instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
        provider.instance_running(instance_id, TEST_REGION)
        exists_one_running_instance(ec2_resource)

    def test_for_stopped(self, provider, ec2_resource):
        instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
        provider.instance_stopped(instance_id, TEST_REGION)
        provider.instance_running(instance_id, TEST_REGION)
        exists_one_running_instance(ec2_resource)

    def test_rollback(self, provider, ec2_resource):
        instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
        provider.instance_stopped(instance_id, TEST_REGION)

        another_provider = new_provider(provider.config)
        another_provider.instance_running(instance_id, TEST_REGION)
        for change in reversed(another_provider.task['changes']):
            change.rollback(provider.task, 42)

        exists_one_stopped_instance(ec2_resource)


class Test_instance_wait_until_stopped(WithMockedSTS):  # noqa
    def test_stopped_instance(self, provider):
        instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
        provider.instance_stopped(instance_id, TEST_REGION)
        provider.instance_wait_until_stopped(instance_id, TEST_REGION)


class Test_update_instance_type(WithMockedSTS):  # noqa
    def test_set_to_same_instance_type(self, provider):
        instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
        provider.instance_stopped(instance_id, TEST_REGION)
        provider.update_instance_type(instance_id, TEST_INSTANCES_SPEC.instance_type, TEST_REGION)

    def test_change_type(self, provider, ec2_resource):
        instance_id = provider.instance_exists(TEST_INSTANCES_SPEC, TEST_REGION)
        provider.instance_stopped(instance_id, TEST_REGION)
        provider.update_instance_type(instance_id, 'yy.test', TEST_REGION)
        assert_that(ec2_resource.Instance(instance_id), has_properties('instance_type', 'yy.test'))


class TestEC2InstanceAddresses(WithMockedSTS):
    addresses_with_public_ip = EC2InstanceAddresses(
        public_v4='192.168.1.10', v6='0:0:dead:beef::', private_v4='10.0.1.2'
    )

    addresses_without_public_ip = EC2InstanceAddresses(public_v4=None, v6='0:0:dead:beef::', private_v4='10.0.1.2')

    def test_public_records_with_public_ip(self):
        assert_that(
            self.addresses_with_public_ip.public_records(),
            equal_to(
                [
                    Record(
                        address='0:0:dead:beef::',
                        record_type='AAAA',
                    ),
                    Record(
                        address='192.168.1.10',
                        record_type='A',
                    ),
                ]
            ),
        )

    def test_private_records_with_public_ip(self):
        assert_that(
            self.addresses_with_public_ip.private_records(),
            equal_to(
                [
                    Record(
                        address='0:0:dead:beef::',
                        record_type='AAAA',
                    ),
                    Record(
                        address='10.0.1.2',
                        record_type='A',
                    ),
                ]
            ),
        )

    def test_public_records_without_public_ip(self):
        assert_that(
            self.addresses_without_public_ip.public_records(),
            equal_to(
                [
                    Record(
                        address='0:0:dead:beef::',
                        record_type='AAAA',
                    ),
                ]
            ),
        )

    def test_private_records_without_public_ip(self):
        assert_that(
            self.addresses_without_public_ip.private_records(),
            equal_to(
                [
                    Record(
                        address='0:0:dead:beef::',
                        record_type='AAAA',
                    ),
                    Record(
                        address='10.0.1.2',
                        record_type='A',
                    ),
                ]
            ),
        )
