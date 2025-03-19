from ....exceptions import ExposedException
from ...common import BaseProvider, Change
from ...metadb_host import MetadbHost
from .instance import EC2InstanceSpec, EC2InstanceAddresses, EC2DiscSpec, EC2InstanceChange

from typing import Any, Callable, Optional, List, Set, NamedTuple
from dbaas_common import retry, tracing
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

import botocore
import botocore.exceptions
from botocore.config import Config
from ..sts import get_assume_role_session_with_role
from ..byoa import BYOA


class EC2APIError(ExposedException):
    pass


class InstanceNotFoundError(EC2APIError):
    pass


class MisconfiguredEnvError(ExposedException):
    pass


class UnsupportedError(EC2APIError):
    pass


class ModificationRateExceeded(EC2APIError):
    pass


class InvalidArnException(EC2APIError):
    pass


class EC2DisabledError(RuntimeError):
    """
    EC2 provider not initialized. Enable it in config'
    """


def _to_gib(size: int) -> int:
    """
    Convert byte size to GiB
    """
    return size // 2**30


def _to_tags(labels: dict) -> List[dict]:
    return [{'Key': k, 'Value': v} for k, v in labels.items()]


def _assert_valid_disk_size(disk_size: int) -> None:
    if disk_size % 2**30:
        raise EC2APIError(f'Disk size should be multiple of 1 GiB. Got {disk_size} bytes')


class _InstanceState:
    # https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/ec2.html#EC2.Instance.state
    pending = 0
    running = 16
    shutting_down = 32
    terminated = 48
    stopping = 64
    stopped = 80


class _InstanceDisk(NamedTuple):
    size: int
    type: str
    volume_id: str
    attachment_state: str
    volume_state: str


# https://docs.aws.amazon.com/AWSEC2/latest/APIReference/API_TagSpecification.html
_TAGGABLE_RESOURCE_TYPES = [
    'network-interface',
    'instance',
    'volume',
]


class _RegionalEC2:
    def __init__(self, config: Any, task: Any, queue: Any, region_name: str, logger: MdbLoggerAdapter) -> None:
        self.config = config
        self.task = task
        self.queue = queue
        self.region_name = region_name
        self.logger = logger

        client_kwargs = dict(
            config=Config(
                retries=self.config.ec2.retries,
            ),
        )
        self.byoa = BYOA(config, task, queue)
        session = get_assume_role_session_with_role(
            role_factory=self.byoa.iam_role,
            aws_access_key_id=self.config.aws.access_key_id,
            aws_secret_access_key=self.config.aws.secret_access_key,
            region_name=self.region_name,
        )
        self._ec2 = session.resource('ec2', **client_kwargs)
        # Some APIs not exist in EC2 Resources,
        # use client for them
        self._ec2_client = session.client('ec2', **client_kwargs)

        default_session = get_assume_role_session_with_role(
            role_factory=lambda: self.config.aws.dataplane_role_arn,
            aws_access_key_id=self.config.aws.access_key_id,
            aws_secret_access_key=self.config.aws.secret_access_key,
            region_name=self.region_name,
        )
        self._default_ec2 = default_session.resource('ec2', **client_kwargs)
        self._default_ec2_client = default_session.client('ec2', **client_kwargs)

    @tracing.trace('Get Instance By ID')
    def get_instance_by_id(self, instance_id: str) -> Any:
        tracing.set_tag('cluster.host.instance_id', instance_id)
        try:
            # It creates proxy-object and doesn't request APIs.
            # That is why we call `load` explicitly.
            inst = self._ec2.Instance(instance_id)
            inst.reload()
            return inst
        except botocore.exceptions.ClientError as client_error:
            if client_error.response['Error']['Code'] == 'InvalidInstanceID.NotFound':
                raise InstanceNotFoundError(f'instance {instance_id} not found') from client_error
            raise

    @retry.on_exception(InstanceNotFoundError, factor=10, max_wait=60, max_tries=6)
    def get_instance_by_id_with_retries(self, instance_id: str) -> Any:
        # Looks like, that EC2 API is not linearizable (ORION-247)
        # That is why we retry NotFound here.
        #
        # https://docs.aws.amazon.com/AWSEC2/latest/APIReference/API_DescribeInstances.html
        #   If you describe instances in the rare case where an Availability Zone is experiencing a service disruption
        #   and you specify instance IDs that are in the affected zone,
        #   or do not specify any instance IDs at all, the call fails.
        #   If you describe instances and specify only instance IDs that are in an unaffected zone,
        #   the call works normally.
        #
        # They don't have details about 'call fails', but our symptoms look similar to it.
        return self.get_instance_by_id(instance_id)

    @tracing.trace('Find Not Terminated Instance By Name')
    def find_not_terminated_instance_id_by_name(self, name: str) -> Optional[str]:
        tracing.set_tag('cluster.host.fqdn', name)

        found = []
        for inst in self._ec2.instances.filter(
            Filters=[
                {'Name': 'tag:Name', 'Values': [name]},
            ]
        ):
            if inst.state['Code'] in (_InstanceState.shutting_down, _InstanceState.terminated):
                self.logger.info(
                    'Find instance %r named %r. And it is in %r state. Ignore it',
                    inst,
                    name,
                    inst.state,
                )
                continue
            found.append(inst)
        if not found:
            return None
        if len(found) > 1:
            raise EC2APIError(f'find more then one instances by name: {name}: {found}')
        return found[0].instance_id

    @tracing.trace('Find Instances By Name')
    def find_instances_by_name(self, name: str) -> List[Any]:
        tracing.set_tag('cluster.host.fqdn', name)

        return list(
            self._ec2.instances.filter(
                Filters=[
                    {'Name': 'tag:Name', 'Values': [name]},
                ]
            )
        )

    @tracing.trace('Choose AMI')
    def _choose_ami_in_account(self, image_type: str, provider: Any) -> Any:
        """
        Choose AMI by 'image_type'
        """
        img_prefix = f'{self.config.ec2.images_prefix}-{image_type}'
        found_image = None
        self.logger.debug('searching for AMI with %r prefix', img_prefix)
        for img in provider.images.filter(
            Filters=[
                {'Name': 'is-public', 'Values': ['false']},
                {'Name': 'state', 'Values': ['available']},
            ]
        ):
            self.logger.debug('examine %r image', img.name)
            if img.name.startswith(img_prefix):
                if found_image is None or found_image.creation_date < img.creation_date:
                    found_image = img
        if found_image is None:
            raise MisconfiguredEnvError(
                f'There are no {image_type!r} AMIs (prefixed with {img_prefix!r}) in {self.region_name} region'
            )
        self.logger.info(
            'Choose %r %r %r for %r image',
            found_image.name,
            found_image.creation_date,
            found_image.image_id,
            image_type,
        )
        return found_image

    def _choose_ami_by_type(self, image_type: str) -> str:
        image = None
        try:
            image = self._choose_ami_in_account(image_type, self._ec2)
            if not self.byoa.is_byoa():
                # it means is default account
                return image.image_id
        except MisconfiguredEnvError as e:
            if not self.byoa.is_byoa():
                # if default account raise error
                raise
            self.logger.info('%s, try to find image in default account', e.message)

        # if BYOA account, check default account to refresh images
        default_image = self._choose_ami_in_account(image_type, self._default_ec2)
        if image and image.creation_date >= default_image.creation_date:
            return image.image_id

        self._share_ami(default_image)
        return default_image.image_id

    @tracing.trace('Share AMI')
    def _share_ami(self, image: Any):
        tracing.set_tag("image_id", image.image_id)
        tracing.set_tag("account_id", self.byoa.account())
        self._default_ec2_client.modify_image_attribute(
            Attribute='launchPermission',
            ImageId=image.image_id,
            LaunchPermission={
                'Add': [
                    {
                        'UserId': self.byoa.account(),
                    },
                ],
            },
        )

    @tracing.trace('Create Instance')
    def create_instance(self, spec: EC2InstanceSpec) -> str:
        """
        Create instance returns its id
        """

        def check_disk(disk: EC2DiscSpec) -> None:
            if disk.type not in self.config.ec2.supported_disk_types:
                raise EC2APIError(
                    f'Unsupported disk_type {disk.type!r}. '
                    'Only {self.config.ec2.supported_disk_types} supported at that moment'
                )

            _assert_valid_disk_size(disk.size)

        check_disk(spec.boot)
        check_disk(spec.data)

        tags = [{'Key': 'Name', 'Value': spec.name}] + _to_tags(spec.labels) + _to_tags(self.config.aws.labels)
        request = dict(
            BlockDeviceMappings=[
                # https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/device_naming.html
                {
                    'DeviceName': '/dev/sda1',
                    'Ebs': {
                        'DeleteOnTermination': True,
                        'VolumeType': spec.boot.type,
                        'VolumeSize': _to_gib(spec.boot.size),
                    },
                },
                {
                    'DeviceName': '/dev/sdf',
                    'Ebs': {
                        'DeleteOnTermination': True,
                        'VolumeType': spec.data.type,
                        'VolumeSize': _to_gib(spec.data.size),
                    },
                },
            ],
            InstanceType=spec.instance_type,
            MaxCount=1,
            MinCount=1,
            ImageId=self._choose_ami_by_type(spec.image_type),
            NetworkInterfaces=[
                {
                    'SubnetId': spec.subnet_id,
                    'AssociatePublicIpAddress': True,
                    'DeviceIndex': 0,
                    'Groups': [spec.sg_id],
                    'Ipv6AddressCount': 1,
                    'DeleteOnTermination': True,
                    'Description': f'{spec.name} public interface',
                },
            ],
            TagSpecifications=[
                {'Tags': tags, 'ResourceType': resource_type} for resource_type in _TAGGABLE_RESOURCE_TYPES
            ],
            UserData=spec.userdata,
            IamInstanceProfile={
                'Arn': spec.iam_instance_profile,
            },
        )
        return self._create_instance(request)

    # create_instances can fail with strange error
    """
    botocore.exceptions.ClientError:
      An error occurred (InvalidParameterValue) when calling the RunInstances operation:
        Value (...) for parameter iamInstanceProfile.arn is invalid.
    """
    # Maybe it fails only just after IAM role creation.
    # But role has no state to check.
    # So, let's retry.
    @retry.on_exception(InvalidArnException, max_tries=5, max_wait=60, factor=10)
    def _create_instance(self, request: dict) -> str:
        self.logger.debug('creating instance %r', request)
        try:
            created = self._ec2.create_instances(**request)
        except botocore.exceptions.ClientError as client_error:
            if (
                client_error.response['Error']['Code'] == 'InvalidParameterValue'
                and 'iamInstanceProfile.arn is invalid' in client_error.response['Error']['Message']
            ):
                raise InvalidArnException(str(client_error))
            raise

        if len(created) != 1:
            raise EC2APIError(f'got more then one instance in create_instances response: {created}')
        inst = created[0]
        self.logger.info('instance created. instance_id: %r. State: %r', inst.instance_id, inst.state)
        return inst.instance_id

    @tracing.trace('Get Instance Data Volume')
    def _get_data_disk(self, inst) -> _InstanceDisk:
        for dev in inst.block_device_mappings:
            if dev['DeviceName'] == inst.root_device_name:
                continue
            volumes = self._ec2_client.describe_volumes(VolumeIds=[dev['Ebs']['VolumeId']])['Volumes']
            if not volumes:
                raise EC2APIError(f'Unable to get volume by it\'s id. Device: {dev}')
            if len(volumes) != 1:
                raise EC2APIError(f'Got more then one volume by id: {volumes}')
            vol = volumes[0]
            return _InstanceDisk(
                size=vol['Size'],
                type=vol['VolumeType'],
                volume_id=vol['VolumeId'],
                attachment_state=dev['Ebs']['Status'],
                volume_state=vol['State'],
            )
        raise EC2APIError(f'Unable to find {inst} data volume. {inst.block_device_mappings=}')

    def gather_instance_changes(
        self, instance_id: str, data_disk: EC2DiscSpec, instance_type: str
    ) -> Set[EC2InstanceChange]:
        instance_changes = set()

        inst = self.get_instance_by_id(instance_id)
        if inst.instance_type != instance_type:
            instance_changes.add(EC2InstanceChange.instance_type)

        actual_disk = self._get_data_disk(inst)
        self.logger.debug('gather_instance_changes got data disk: %r', actual_disk)

        _assert_valid_disk_size(data_disk.size)

        want_size = _to_gib(data_disk.size)
        if want_size != actual_disk.size:
            if want_size > actual_disk.size:
                instance_changes.add(EC2InstanceChange.disk_size_up)
            else:
                raise UnsupportedError(f'Disk size scale down not supported ({actual_disk.size}GiB -> {want_size}GiB)')

        if data_disk.type != actual_disk.type:
            raise UnsupportedError(f'Disk type changes not supported ({actual_disk.type} -> {data_disk.type})')

        return instance_changes

    def set_instance_data_disk_size(self, instance_id: str, data_disk_size: int) -> Optional[str]:
        inst = self.get_instance_by_id(instance_id)
        actual_disk = self._get_data_disk(inst)
        self.logger.debug('set_instance_data_disk_size got data disk: %r', actual_disk)

        want_size = _to_gib(data_disk_size)
        if want_size == actual_disk.size:
            self.logger.info('attempt to update %r data volume size, but it\'s already %dGib', instance_id, want_size)
            return None
        try:
            response = self._ec2_client.modify_volume(
                VolumeId=actual_disk.volume_id,
                Size=want_size,
            )
            self.logger.info('modify volume response %r', response)
        except botocore.exceptions.ClientError as client_error:
            if client_error.response['Error']['Code'] == 'VolumeModificationRateExceeded':
                self.logger.warning('VolumeModificationRateExceeded: %s', client_error)
                raise ModificationRateExceeded(client_error.response['Error']['Message']) from client_error
            raise
        return actual_disk.volume_id

    def wait_for_volume_resize(self, volume_id: str) -> None:
        # By default its wait for 600s.
        #
        # Polls EC2.Client.describe_volumes() every 15 seconds until a successful state is reached.
        # An error is returned after 40 failed checks.

        waiter = self._ec2_client.get_waiter('volume_in_use')
        waiter.wait(VolumeIds=[volume_id])
        #
        # For initial start it should be ok, but number of checks
        # should be increased if we decide to support disk_type or IOPs modification.
        #
        #  Volume modification changes take effect as follows:
        # - Size changes usually take a few seconds to complete
        #   and take effect after the volume has transitioned to the Optimizing state.
        # - Performance (IOPS) changes can take from a few minutes to a few hours to complete
        #   and are dependent on the configuration change being made.
        # - It might take up to 24 hours for a new configuration to take effect,
        #   and in some cases more, such as when the volume has not been fully initialized.
        #   Typically, a fully used 1-TiB volume takes about 6 hours to migrate to a new performance configuration.
        #
        # https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/monitoring-volume-modifications.html

    def update_instance_type(self, instance_id: str, instance_type: str) -> None:
        inst = self.get_instance_by_id(instance_id)
        if inst.instance_type == instance_type:
            self.logger.info(
                'instance %r (state: %r) already has instance type %r', instance_id, inst.state, instance_type
            )
            return
        self.logger.info('changing %r instance type from %r to %r', instance_id, inst.instance_type, instance_type)
        inst.modify_attribute(
            InstanceType={
                'Value': instance_type,
            }
        )

    def instance_running(self, instance_id: str) -> None:
        inst = self.get_instance_by_id(instance_id)
        if inst.state['Code'] in (_InstanceState.running, _InstanceState.pending):
            self.logger.info('instance %r already running (or pending): %r', instance_id, inst.state)
            return

        inst.start()

    def instance_wait_until_running(self, instance_id: str) -> None:
        inst = self.get_instance_by_id_with_retries(instance_id)
        if inst.state['Code'] == _InstanceState.stopping:
            inst.wait_until_stopped()
            inst.reload()
        if inst.state['Code'] == _InstanceState.stopped:
            inst.start()
            inst.reload()
        inst.wait_until_running()

    def instance_stopped(self, instance_id: str) -> None:
        inst = self.get_instance_by_id(instance_id)

        if inst.state['Code'] in (_InstanceState.stopped, _InstanceState.stopping):
            self.logger.info('instance %r already stopped(ing): %r', instance_id, inst.state)
            return

        inst.stop()

    def instance_wait_until_stopped(self, instance_id: str) -> None:
        inst = self.get_instance_by_id(instance_id)
        inst.wait_until_stopped()


class EC2(BaseProvider):
    _regional_ec2 = None

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        if config.ec2.enabled:
            self.metadb_host = MetadbHost(config, task, queue)
            self._regional_ec2 = {}

    def _get_regional_ec2(self, region_name: str) -> _RegionalEC2:
        if self._regional_ec2 is None:
            raise EC2DisabledError

        if region_name not in self._regional_ec2:
            self._regional_ec2[region_name] = _RegionalEC2(
                self.config,
                self.task,
                self.queue,
                region_name,
                self.logger,
            )
        return self._regional_ec2[region_name]

    def _not_present_in_context(
        self, key: str, change_name: str, change_value: str, rollback: Callable[[Any, Any], None] = None
    ) -> Optional[Callable[[], None]]:
        """
        Returns callable that add Change if 'key' not present in context.

        Useful if:
            1. caller use one context key
            2. caller don't use value store in context
        """
        if self.context_get(key):
            self.add_change(Change(change_name, change_value + ' early'))
            return None

        return lambda: self.add_change(Change(change_name, change_value, context={key: 'called'}, rollback=rollback))

    @tracing.trace('Instance Exists')
    def instance_exists(self, spec: EC2InstanceSpec, region_name: str) -> str:
        regional_ec2 = self._get_regional_ec2(region_name)
        tracing.set_tag('cluster.host.fqdn', spec.name)

        change = (f'instance.{spec.name}', 'created')
        context_key = f'{spec.name}.instance_create'
        context = self.context_get(context_key)
        if context:
            self.add_change(Change(*change))
            return context['id']

        instance_id = regional_ec2.find_not_terminated_instance_id_by_name(spec.name)
        if not instance_id:
            instance_id = regional_ec2.create_instance(spec)

        self.add_change(Change(*change, {context_key: {'id': instance_id}}))
        return instance_id

    @tracing.trace('Instance Wait Until Running')
    def instance_wait_until_running(self, instance_id: str, region_name: str) -> None:
        tracing.set_tag('cluster.host.instance_id', instance_id)
        regional_ec2 = self._get_regional_ec2(region_name)

        if change := self._not_present_in_context(
            key=f'wait.until.running.instance.{instance_id}',
            change_name=f'instance.{instance_id}',
            change_value='running',
            rollback=Change.noop_rollback,
        ):
            regional_ec2.instance_wait_until_running(instance_id)
            change()

    @tracing.trace('Save Instance Meta in MetaDB')
    def save_instance_meta(self, instance_id: str, name: str) -> None:
        """
        Currently its only instance_id
        """
        self.metadb_host.update(name, 'vtype_id', instance_id)

    @tracing.trace('Save Instance subnet in MetaDB')
    def save_instance_subnet(self, subnet_id: str, fqdn: str) -> None:
        """
        Currently its only instance_id
        """

        self.metadb_host.update(fqdn, 'subnet_id', subnet_id)

    @tracing.trace('Instance Absent')
    def instance_absent(self, name: str, region_name: str) -> List[str]:
        """
        Terminate instance.
        Do nothing if instance is terminating or already terminated
        """
        regional_ec2 = self._get_regional_ec2(region_name)
        tracing.set_tag('cluster.host.fqdn', name)
        terminating = []

        for inst in regional_ec2.find_instances_by_name(name):
            if inst.state['Code'] == _InstanceState.terminated:
                self.logger.info('instance %r already terminated', inst)
                continue
            if inst.state['Code'] != _InstanceState.shutting_down:
                self.add_change(Change(f'instance.{inst.instance_id}', 'terminated'))
                inst.terminate()
            terminating.append(inst.instance_id)
        return terminating

    @tracing.trace('Instance Stopped')
    def instance_stopped(self, instance_id: str, region_name: str) -> None:
        """
        Stop instance.
        Do nothing if instance is stopping or already stopped.
        """
        tracing.set_tag('cluster.host.instance_id', instance_id)
        regional_ec2 = self._get_regional_ec2(region_name)

        if change := self._not_present_in_context(
            key=f'stop.instance.{instance_id}',
            change_name=f'instance.{instance_id}',
            change_value='stop initiated',
            rollback=lambda task, safe_revision: regional_ec2.instance_running(instance_id),
        ):
            regional_ec2.instance_stopped(instance_id)
            change()

    @tracing.trace('Instance Running')
    def instance_running(self, instance_id: str, region_name: str) -> None:
        """
        Run instance.
        Do nothing if instance is running or pending
        """
        tracing.set_tag('cluster.host.instance_id', instance_id)
        regional_ec2 = self._get_regional_ec2(region_name)

        if change := self._not_present_in_context(
            key=f'start.instance.{instance_id}',
            change_name=f'instance.{instance_id}',
            change_value='start initiated',
            rollback=lambda task, safe_revision: regional_ec2.instance_stopped(instance_id),
        ):
            regional_ec2.instance_running(instance_id)
            change()

    @tracing.trace('Instance Wait Until Terminated')
    def instance_wait_until_terminated(self, instance_id: str, region_name: str) -> None:
        regional_ec2 = self._get_regional_ec2(region_name)
        tracing.set_tag('cluster.host.instance_id', instance_id)

        try:
            inst = regional_ec2.get_instance_by_id(instance_id)
        except InstanceNotFoundError:
            self.logger.info('instance %r not exists', instance_id)
            return
        inst.wait_until_terminated()

    @tracing.trace('Instance Wait Until Stopped')
    def instance_wait_until_stopped(self, instance_id: str, region_name: str) -> None:
        regional_ec2 = self._get_regional_ec2(region_name)
        tracing.set_tag('cluster.host.instance_id', instance_id)

        if change := self._not_present_in_context(
            key=f'wait.until.stopped.instance.{instance_id}',
            change_name=f'instance.{instance_id}',
            change_value='stopped',
            rollback=Change.noop_rollback,
        ):
            regional_ec2.instance_wait_until_stopped(instance_id)
            change()

    @tracing.trace('Get Instance Addresses')
    def get_instance_addresses(self, instance_id: str, region_name: str) -> EC2InstanceAddresses:
        regional_ec2 = self._get_regional_ec2(region_name)
        tracing.set_tag('cluster.host.instance_id', instance_id)

        inst = regional_ec2.get_instance_by_id_with_retries(instance_id)
        v6_addresses = []
        for interface in inst.network_interfaces:
            if interface.ipv6_addresses:
                for address in interface.ipv6_addresses:
                    v6_addresses.append(address['Ipv6Address'])
        if not v6_addresses:
            raise EC2APIError(f'Instance {instance_id} does not have v6 address. Interfaces: {inst.network_interfaces}')
        if len(v6_addresses) > 1:
            raise EC2APIError(f'Instance {instance_id} has more then one v6 address: {v6_addresses}')
        return EC2InstanceAddresses(
            public_v4=inst.public_ip_address if inst.public_ip_address else None,
            v6=v6_addresses[0],
            private_v4=inst.private_ip_address,
        )

    @tracing.trace('Get Instance Changes')
    def get_instance_changes(
        self, instance_id: str, data_disk: EC2DiscSpec, instance_type: str, region_name: str
    ) -> Set[EC2InstanceChange]:
        regional_ec2 = self._get_regional_ec2(region_name)
        tracing.set_tag('cluster.host.instance_id', instance_id)

        change_id = f'{instance_id}.ec2_changes'
        changes_from_context = self.context_get(change_id)
        if changes_from_context is not None:
            self.add_change(Change(change_id, list(changes_from_context), rollback=Change.noop_rollback))
            return set(EC2InstanceChange[c] for c in changes_from_context)

        instance_changes = regional_ec2.gather_instance_changes(instance_id, data_disk, instance_type)
        serializable_instance_changes = [c.name for c in instance_changes]
        self.add_change(
            Change(
                change_id,
                serializable_instance_changes,
                context={change_id: serializable_instance_changes},
                rollback=Change.noop_rollback,
            )
        )
        return instance_changes

    @tracing.trace('Update Instance Data Disk Size')
    def set_instance_data_disk_size(self, instance_id: str, data_disk_size: int, region_name: str) -> None:
        regional_ec2 = self._get_regional_ec2(region_name)
        tracing.set_tag('cluster.host.instance_id', instance_id)

        # https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/requesting-ebs-volume-modifications.html
        # > You can only increase volume size.
        #
        # So that change doesn't have rollback
        volume_id = regional_ec2.set_instance_data_disk_size(instance_id, data_disk_size)
        if volume_id is not None:
            self.add_change(Change(f'{instance_id}.disk_resize', 'initiated'))
            regional_ec2.wait_for_volume_resize(volume_id)
        self.add_change(Change(f'{instance_id}.disk_resize', 'finished'))

    @tracing.trace('Update Instance Type')
    def update_instance_type(self, instance_id: str, instance_type, region_name: str) -> None:
        """
        Update instance type
        """
        regional_ec2 = self._get_regional_ec2(region_name)
        tracing.set_tag('cluster.host.instance_id', instance_id)

        if change := self._not_present_in_context(
            key=f'{instance_id}.update_instance_type',
            change_name=f'{instance_id}.update_instance_type',
            change_value='initiated',
        ):
            regional_ec2.update_instance_type(instance_id, instance_type)
            change()
