"""
YC.Compute interaction module
"""
import time
import uuid
from typing import Callable, List, Sequence, Type, Optional, Iterable

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.disks import DiskModel, DisksClient, DisksClientConfig
from cloud.mdb.internal.python.compute.host_groups import HostGroupModel, HostGroupsClient, HostGroupsClientConfig
from cloud.mdb.internal.python.compute.host_type import HostTypeModel, HostTypeClient, HostTypeClientConfig
from cloud.mdb.internal.python.compute.images import ImageModel, ImageStatus, ImagesClient, ImagesClientConfig
from cloud.mdb.internal.python.compute.instances import (
    AttachedDiskRequest,
    ConfigurationError,
    DnsRecordSpec,
    InstanceDnsSpec,
    IPV6OnlyPublicIPError,
    InstanceModel,
    InstanceStatus,
    InstanceView,
    InstancesClient,
    InstancesClientConfig,
    nvme_chunk_size,
    PLATFORM_MAP,
    NetworkInterface,
    NetworkType,
    SubnetRequest,
    Referrer,
    ReferenceType,
    Reference,
    ReferrerType,
)
from cloud.mdb.internal.python.compute.models import OperationModel
from cloud.mdb.internal.python.compute.operations import (
    OperationsClient,
    OperationsClientConfig,
)
from cloud.mdb.internal.python.grpcutil import exceptions as grpcutil_errors
from dbaas_common import retry, tracing

from .common import Change
from .http import BaseProvider
from .iam_jwt import IamJwt
from .metadb_placement_group import MetadbPlacementGroup
from .metadb_disk import MetadbDisks
from .metadb_disk_placement_group import MetadbDiskPlacementGroup
from .metadb_host import MetadbHost
from .vpc import VPCProvider
from cloud.mdb.internal.python.compute import vpc
from ..exceptions import ExposedException, UserExposedException


MDB_CMS_REFERRER_ID = 'mdb.cms'


def _get_compute_name_from_fqdn(fqdn):
    """
    Compute does not use fqdn as instance name.
    So we strip domain name from fqdn here
    """
    return fqdn.split('.', maxsplit=1)[0]


# pylint: disable=too-many-return-statements
def check_resources_changes(instance: InstanceModel, platform_id, cores, core_fraction, gpus, io_cores, memory):
    """
    Check if there are any resource changes for given instance
    """
    if instance.platform_id != platform_id:
        return True
    if instance.resources.cores != int(cores):
        return True
    if instance.resources.memory != memory:
        return True
    if instance.resources.core_fractions != int(core_fraction):
        return True
    if instance.resources.gpus != int(gpus):
        return True
    # We shoud not trigger if io_cores changed from 2 to 4. We should trigger changes only if
    # number of io_cores changed from zero to non-zero or vice versa
    if instance.network_settings.type == NetworkType.SOFTWARE_ACCELERATED and io_cores == 0:
        return True
    if instance.network_settings.type != NetworkType.SOFTWARE_ACCELERATED and io_cores > 0:
        return True

    return False


def check_security_groups_changes(
    instance: InstanceModel, network: vpc.Network, desired_security_groups: Iterable
) -> bool:
    """
    Check if there are security_groups changes for given instance
    """
    if not len(instance.network_interfaces):
        return False
    interface = instance.network_interfaces[0]
    current_security_groups = set()
    if hasattr(interface, 'security_group_ids'):
        current_security_groups = set(interface.security_group_ids)
    if len(current_security_groups) == 0 and network.default_security_group_id:
        current_security_groups.add(network.default_security_group_id)
    if desired_security_groups is None:
        desired_security_groups = []
    if set(desired_security_groups) != current_security_groups:
        return True

    return False


def get_core_fraction(cpu_guarantee, cpu_limit):
    """
    Calculate core fraction from cpu_guarantee and cpu_limit
    """
    return round(cpu_guarantee / cpu_limit * 100)


class ComputeApiError(ExposedException):
    """
    Base compute error
    """


class UserExposedComputeApiError(UserExposedException):
    """
    Compute error we are ready to show to user
    """


class UserExposedComputeRunningOperationsLimitError(UserExposedComputeApiError):
    """
    User compute max running operations limit reached error
    """


def quota_error(err: grpcutil_errors.ResourceExhausted) -> None:
    if err.message == 'The limit on maximum number of active operations has exceeded.':
        raise UserExposedComputeRunningOperationsLimitError(message=err.message, err_type=err.err_type, code=err.code)
    raise UserExposedComputeApiError(message=err.message, err_type=err.err_type, code=err.code)


def gen_config(ca_path: str) -> Callable:
    def from_url(url: str) -> grpcutil.Config:
        return grpcutil.Config(
            url=url,
            cert_file=ca_path,
        )

    return from_url


def retry_operations_limit_exceeded(f):
    def wrapper(*args, **kwargs):
        if not args[0].should_retry_operations_limit_exceeded:
            return f(*args, **kwargs)

        return retry.on_exception(
            UserExposedComputeRunningOperationsLimitError,
            factor=1,
            max_wait=3600,
            max_tries=20,
        )(f)(*args, **kwargs)

    return wrapper


class ComputeApi(BaseProvider):
    """
    Compute provider
    """

    INSTANCE_DELETED_STATE = 'DELETED'

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.metadb_host = MetadbHost(config, task, queue)
        self.metadb_disk = MetadbDisks(config, task, queue)
        self.metadb_disk_placement_group = MetadbDiskPlacementGroup(config, task, queue)
        self.metadb_placement_group = MetadbPlacementGroup(config, task, queue)
        self._address_cache = dict()
        self._instance_id_cache = dict()
        self._idempotence_ids = dict()
        self.should_retry_operations_limit_exceeded = False
        self.iam_jwt = IamJwt(
            config,
            task,
            queue,
            service_account_id=self.config.compute.service_account_id,
            key_id=self.config.compute.key_id,
            private_key=self.config.compute.private_key,
        )

        transport_config = gen_config(self.config.compute.ca_path)
        error_handlers = {
            grpcutil_errors.ResourceExhausted: quota_error,
        }
        self.__instances_client = InstancesClient(
            config=InstancesClientConfig(
                transport=transport_config(self.config.compute.url),
            ),
            logger=self.logger,
            token_getter=self.get_token,
            error_handlers=error_handlers,
        )
        self.__images_client = ImagesClient(
            config=ImagesClientConfig(
                transport=transport_config(self.config.compute.url),
            ),
            logger=self.logger,
            token_getter=self.get_token,
            error_handlers=error_handlers,
        )
        self.__disks_client = DisksClient(
            config=DisksClientConfig(
                transport=transport_config(self.config.compute.url),
            ),
            logger=self.logger,
            token_getter=self.get_token,
            error_handlers=error_handlers,
        )
        self.__operations_client = OperationsClient(
            config=OperationsClientConfig(
                transport=transport_config(self.config.compute.url),
            ),
            logger=self.logger,
            token_getter=self.get_token,
            error_handlers=error_handlers,
        )
        self.__host_group_client = HostGroupsClient(
            config=HostGroupsClientConfig(
                transport=transport_config(self.config.compute.url),
            ),
            logger=self.logger,
            token_getter=self.get_token,
            error_handlers=error_handlers,
        )
        self.__host_type_client = HostTypeClient(
            config=HostTypeClientConfig(
                transport=transport_config(self.config.compute.url),
            ),
            logger=self.logger,
            token_getter=self.get_token,
            error_handlers=error_handlers,
        )
        self._vpc_provider = VPCProvider(config, task, queue)

    def get_token(self):
        return self.iam_jwt.get_token()

    def _get_idempotence_id(self, key):
        """
        Get local idempotence id by key
        """
        if key not in self._idempotence_ids:
            self._idempotence_ids[key] = str(uuid.uuid4())
        return self._idempotence_ids[key]

    def list_instances(self, folder_id: str = None, name: str = None) -> Sequence[InstanceModel]:
        """
        Get instances in folder
        """
        return self.__instances_client.list_instances(
            folder_id or self.config.compute.folder_id,
            name=name,
        )

    def list_disks(self) -> Sequence[Type[DiskModel]]:
        """
        Get disks in folder
        """
        return self.__disks_client.list_disks(self.config.compute.folder_id)

    @retry.on_exception(ComputeApiError, factor=10, max_wait=60, max_tries=6)
    def get_host_group(self, host_group_id) -> HostGroupModel:
        """
        Get host group
        """
        return self.__host_group_client.get_host_group(host_group_id)

    @retry.on_exception(ComputeApiError, factor=10, max_wait=60, max_tries=6)
    def get_host_type(self, host_type_id) -> HostTypeModel:
        """
        Get host type
        """
        return self.__host_type_client.get_host_type(host_type_id)

    @retry.on_exception(ComputeApiError, factor=10, max_wait=60, max_tries=6)
    def _get_subnet(self, subnet_id):
        """
        Get subnet by id
        """
        return self._vpc_provider.get_subnet(subnet_id)

    @retry.on_exception(ComputeApiError, factor=10, max_wait=60, max_tries=6)
    def _get_geo_subnet(self, folder_id, network_id, zone_id):
        """
        Get first subnet for folder/network/zone combination
        """
        return self._vpc_provider.get_geo_subnet(folder_id, network_id, zone_id)

    def _get_latest_image(self, image_type) -> ImageModel:
        """
        Get latest (by date) ready image
        """
        latest = None

        search_pattern = 'dbaas-{type}-image-'.format(type=image_type)
        for image in self.__images_client.list_images(self.config.compute.folder_id):
            if image.description.startswith(search_pattern):
                if image.status != ImageStatus.READY:
                    continue
                if latest is None:
                    latest = image
                elif image.created_at > latest.created_at:
                    latest = image

        if latest is None:
            raise ComputeApiError(
                'Image for \'{type}\', folder \'{folder}\' not found'.format(
                    type=image_type, folder=self.config.compute.folder_id
                )
            )

        return latest

    def _get_image(self, image_id) -> ImageModel:
        """
        Get image by ID
        """
        return self.__images_client.get_image(image_id)

    def get_instance(
        self, fqdn: Optional[str], instance_id: str = None, folder_id: str = None, view='BASIC'
    ) -> Optional[InstanceModel]:
        """
        Get instance info if exists
        """
        with self.logger.context(method='get_instance', fqdn=fqdn, instance_id=instance_id):
            instance_id = instance_id or self._instance_id_cache.get(fqdn)
            if not instance_id:
                host_info = self.metadb_host.get_host_info(fqdn)
                if host_info:
                    instance_id = host_info['vtype_id']
            if instance_id:
                try:
                    res = self.__instances_client.get_instance(instance_id, InstanceView[view])
                except grpcutil_errors.NotFoundError as exc:
                    self.logger.info('Unable to get instance %s by id: %s', fqdn, repr(exc))
                else:
                    if fqdn and not self._instance_id_cache.get(fqdn):
                        self._instance_id_cache[fqdn] = res.id
                    return res

            if fqdn:
                instances = self.list_instances(
                    folder_id=folder_id or self.config.compute.folder_id,
                    name=_get_compute_name_from_fqdn(fqdn),
                )
                for instance in instances:
                    if instance.fqdn == fqdn:
                        self._instance_id_cache[fqdn] = instance.id
                        if view != 'BASIC':
                            return self.get_instance(None, instance_id=instance.id, folder_id=folder_id, view=view)
                        return instance
        return None

    def _get_disk(self, disk_id) -> Optional[DiskModel]:
        """
        Get disk by id
        """
        try:
            return self.__disks_client.get_disk(disk_id)
        except grpcutil_errors.NotFoundError:
            return None

    @tracing.trace('Compute Operation Get', ignore_active_span=True)
    def _get_operation(self, operation_id) -> OperationModel:
        """
        Get operation by id
        """
        return self.__operations_client.get_operation(operation_id)

    def _get_subnet_requests(self, subnet_id, geo) -> List[SubnetRequest]:
        """
        Generate interfaces spec for instance
        """
        subnets = [
            self._get_subnet(subnet_id),
        ]
        if self.config.compute.managed_network_id:
            subnets.append(
                self._get_geo_subnet(
                    self.config.compute.folder_id,
                    self.config.compute.managed_network_id,
                    self.config.compute.geo_map.get(geo, geo),
                )
            )
        interfaces = []
        for subnet in subnets:
            interfaces.append(
                SubnetRequest(
                    v4_cidr=bool(subnet.v4_cidr_blocks),
                    v6_cidr=bool(subnet.v6_cidr_blocks),
                    subnet_id=subnet.subnet_id,
                )
            )

        return interfaces

    # pylint: disable=too-many-arguments,too-many-locals
    @retry_operations_limit_exceeded
    def _create_instance(
        self,
        geo: str,
        fqdn: str,
        managed_fqdn: str,
        image: ImageModel,
        platform_id: str,
        cores: int,
        core_fraction: float,
        gpus: int,
        io_cores: int,
        memory: int,
        subnet_id: str,
        assign_public_ip: bool,
        security_groups: list[str],
        disk_type_id: str,
        disk_size: int,
        root_disk_size: int,
        root_disk_type_id: str,
        folder_id: str,
        metadata: str,
        service_account_id: Optional[str],
        idempotence_id: str,
        labels: dict,
        host_group_ids: list[str],
        disk_placement_group_id: str = None,
        vhost_net: bool = False,
        local_disk_chunk_size: int = None,
        placement_group_id: str = None,
        **kwargs,
    ) -> OperationModel:
        if root_disk_size is not None:
            boot_disk_size = root_disk_size
        else:
            boot_disk_size = image.min_disk_size
        boot_disk_spec = None
        if kwargs.get('bootDiskSpec'):
            spec = kwargs.get('bootDiskSpec')
            boot_disk_spec = AttachedDiskRequest(disk_id=spec['diskId'])  # type: ignore

        secondary_disk_specs = []
        if kwargs.get('secondaryDiskSpecs'):
            for spec in kwargs.get('secondaryDiskSpecs'):  # type: ignore
                secondary_disk_specs.append(AttachedDiskRequest(disk_id=spec['diskId']))

        references = []
        if kwargs.get('references'):
            for reference in kwargs['references']:  # type: ignore
                references.append(
                    Reference(
                        referrer=Referrer(type=ReferrerType(reference['referrer_type']), id=reference['referrer_id']),
                        type=ReferenceType.MANAGED_BY,
                    )
                )

        is_mdb_cms_managed = folder_id == self.config.compute.folder_id
        if is_mdb_cms_managed:
            references.append(
                Reference(
                    referrer=Referrer(type=ReferrerType.MDB_CMS, id=MDB_CMS_REFERRER_ID),
                    type=ReferenceType.MANAGED_BY,
                )
            )

        managed_dns_record: Optional[DnsRecordSpec] = None
        if self.config.compute.managed_dns_zone_id:
            managed_dns_record = DnsRecordSpec(
                fqdn=managed_fqdn,
                dns_zone_id=self.config.compute.managed_dns_zone_id,
                ttl=self.config.compute.managed_dns_ttl,
                ptr=self.config.compute.managed_dns_ptr,
            )
        user_dns_record: Optional[DnsRecordSpec] = None
        if self.config.compute.user_dns_zone_id:
            user_dns_record = DnsRecordSpec(
                fqdn=fqdn,
                dns_zone_id=self.config.compute.user_dns_zone_id,
                ttl=self.config.compute.user_dns_ttl,
                ptr=self.config.compute.user_dns_ptr,
            )

        try:
            operation = self.__instances_client.create_instance(
                zone_id=self.config.compute.geo_map.get(geo, geo),
                fqdn=fqdn,
                boot_disk_size=boot_disk_size,
                image_id=image.id,
                platform_id=platform_id,
                cores=int(cores),
                core_fraction=int(core_fraction),
                gpus=int(gpus),
                io_cores=int(io_cores),
                memory=int(memory),
                assign_public_ip=assign_public_ip,
                security_group_ids=security_groups,
                disk_type_id=disk_type_id,
                disk_size=disk_size,
                boot_disk_type_id=root_disk_type_id,
                folder_id=folder_id,
                metadata=metadata,
                service_account_id=service_account_id,
                idempotence_id=idempotence_id,
                name=_get_compute_name_from_fqdn(fqdn),
                secondary_disk_specs=secondary_disk_specs,
                boot_disk_spec=boot_disk_spec,
                subnets=self._get_subnet_requests(subnet_id, geo),
                labels=labels,
                references=references,
                host_group_ids=host_group_ids,
                disk_placement_group_id=disk_placement_group_id,
                vhost_net=vhost_net,
                local_disk_chunk_size=local_disk_chunk_size,
                placement_group_id=placement_group_id,
                dns_spec=InstanceDnsSpec(eth0_ip6=user_dns_record, eth1_ip6=managed_dns_record),
            )
        except IPV6OnlyPublicIPError:
            raise UserExposedComputeApiError(
                message='Public ip assignment in ipv6-only networks is not supported',
                err_type='UNIMPLEMENTED',
                code=12,
            )
        except ConfigurationError as exc:
            raise ComputeApiError(exc.args[0])
        else:
            return operation

    @retry_operations_limit_exceeded
    def _delete_instance(self, instance_id, idempotence_id, referrer_id=MDB_CMS_REFERRER_ID):
        """
        Delete existing instance
        """
        return self.__instances_client.delete_instance(instance_id, idempotence_id, referrer_id=referrer_id)

    @retry_operations_limit_exceeded
    def _start_instance(self, instance_id, idempotence_id, referrer_id=MDB_CMS_REFERRER_ID) -> OperationModel:
        """
        Start stopped instance
        """
        return self.__instances_client.start_instance(instance_id, idempotence_id, referrer_id=referrer_id)

    @retry_operations_limit_exceeded
    def _stop_instance(
        self,
        instance_id: str,
        termination_grace_period: Optional[int],
        idempotence_id: str,
        referrer_id: str = MDB_CMS_REFERRER_ID,
    ) -> OperationModel:
        """
        Stop running instance
        """
        return self.__instances_client.stop_instance(
            instance_id, termination_grace_period, idempotence_id, referrer_id=referrer_id
        )

    def _rollback_fun_from_desc(self, rollback_desc):
        """
        Build rollback function from description
        """
        operation = rollback_desc.get('operation')
        if operation == 'instance-delete':
            target = rollback_desc.get('target')
            if not target:
                raise ComputeApiError(f'Got instance-delete operation without target: {rollback_desc}')

            return lambda task, safe_revision: self.instance_absent(target)
        raise ComputeApiError(f'Unexpected rollback description: {rollback_desc}')

    def disable_billing(self, fqdn: str) -> str:
        """
        Disable billing for instance.
        """
        instance = self.get_instance(fqdn)
        if instance is None:
            raise RuntimeError(f'Instance {fqdn} not found by its FQDN')
        operation = self.__instances_client.update_metadata(
            instance_id=instance.id,
            metadata={'billing-disabled': 'true'},
            delete_fields=[],
            idempotency_key=self._get_idempotence_id(f'{fqdn}.billing_disable'),
        )
        return operation.operation_id

    def enable_billing(self, fqdn: str) -> Optional[str]:
        """
        Enable billing for instance.
        """
        instance = self.get_instance(fqdn, view='FULL')
        if instance is None:
            raise RuntimeError(f'Instance {fqdn} not found by its FQDN')
        metadata = instance.metadata
        if 'billing-disabled' in metadata:
            operation = self.__instances_client.update_metadata(
                instance_id=instance.id,
                metadata={},
                delete_fields=['billing-disabled'],
                idempotency_key=self._get_idempotence_id(f'{fqdn}.billing_enable'),
            )
            return operation.operation_id
        self.logger.info('Skipping billing enable as no disable flag in metadata')
        return None

    def attach_disk(self, instance_id, disk_id):
        """
        Attach disk to instance
        """
        operation_from_context = self.context_get(f'{instance_id}.{disk_id}.attach')
        if operation_from_context:
            self.add_change(Change(f'{instance_id}.disk.{disk_id}', 'attach initiated'))
            return operation_from_context
        instance = self.get_instance(None, instance_id=instance_id)
        instance_disks = {x.disk_id for x in instance.secondary_disks}
        if disk_id in instance_disks:
            return
        operation = self.__instances_client.attach_disk(
            instance_id,
            disk_id,
            idempotency_key=self._get_idempotence_id(f'{instance_id}.{disk_id}.attach'),
        )
        self.add_change(
            Change(
                f'{instance_id}.disk.{disk_id}',
                'attach initiated',
                context={f'{instance_id}.{disk_id}.attach': operation.operation_id},
            )
        )
        return operation.operation_id

    def detach_disk(self, instance_id, disk_id):
        """
        Detach disk from instance
        """
        operation_from_context = self.context_get(f'{instance_id}.{disk_id}.detach')
        if operation_from_context:
            self.add_change(Change(f'{instance_id}.disk.{disk_id}', 'detach initiated'))
            return operation_from_context
        instance = self.get_instance(None, instance_id=instance_id)
        instance_disks = {x.disk_id for x in instance.secondary_disks}
        if disk_id not in instance_disks:
            return
        operation = self.__instances_client.detach_disk(
            instance_id,
            disk_id,
            idempotency_key=self._get_idempotence_id(f'{instance_id}.{disk_id}.detach'),
        )
        self.add_change(
            Change(
                f'{instance_id}.disk.{disk_id}',
                'detach initiated',
                context={f'{instance_id}.{disk_id}.detach': operation.operation_id},
            )
        )
        return operation.operation_id

    def set_autodelete_for_boot_disk(self, instance_id, boot_disk_id, autodelete_flag):
        """
        Set no auto delete mark on disk
        """
        context_key = f'{instance_id}.{boot_disk_id}.autodelete.{autodelete_flag}'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(context_key, 'set initiated'))
            return operation_from_context
        operation = self.__instances_client.update_instance(
            instance_id=instance_id,
            idempotency_key=self._get_idempotence_id(context_key),
            disk_id=boot_disk_id,
            autodelete_flag=autodelete_flag,
        )
        self.add_change(
            Change(
                context_key,
                'set initiated',
                context={context_key: operation.operation_id},
                rollback=lambda task, safe_revision: self.set_autodelete_for_boot_disk(
                    instance_id, boot_disk_id, not autodelete_flag
                ),
            )
        )
        return operation.operation_id

    def delete_disk(self, disk_id):
        """
        Delete disk by id
        """
        operation_from_context = self.context_get(f'disk.{disk_id}.delete')
        if operation_from_context:
            self.add_change(Change(f'disk.{disk_id}', 'delete initiated'))
            return operation_from_context
        operation = self.__disks_client.delete_disk(
            disk_id,
            idempotency_key=self._get_idempotence_id(f'disk.{disk_id}.delete'),
        )
        self.add_change(
            Change(f'disk.{disk_id}', 'delete initiated', context={f'disk.{disk_id}.delete': operation.operation_id})
        )
        return operation.operation_id

    @retry_operations_limit_exceeded
    def set_disk_size(self, disk_id, size):
        """
        Set disk size by id
        """
        operation_from_context = self.context_get(f'disk.{disk_id}.resize')
        if operation_from_context:
            self.add_change(Change(f'disk.{disk_id}', 'resize initiated'))
            return operation_from_context
        operation = self.__disks_client.set_disk_size(
            disk_id,
            size,
            idempotency_key=self._get_idempotence_id(f'disk.{disk_id}.resize'),
        )
        self.add_change(
            Change(f'disk.{disk_id}', 'resize initiated', context={f'disk.{disk_id}.resize': operation.operation_id})
        )
        return operation.operation_id

    def create_disk(self, geo, size, type_id, context_key=None, image_id=None, disk_placement_group_id: str = None):
        """
        Create disk
        """
        if context_key:
            disk_from_context = self.context_get(context_key)
            if disk_from_context:
                self.add_change(Change(f'disk.{disk_from_context["disk_id"]}', 'create initiated'))
                return disk_from_context['operation_id'], disk_from_context['disk_id']
        created_disk = self.__disks_client.create_disk(
            geo=geo,
            size=size,
            type_id=type_id,
            folder_id=self.config.compute.folder_id,
            zone_id=self.config.compute.geo_map.get(geo, geo),
            idempotency_key=self._get_idempotence_id(context_key),
            image_id=image_id,
            disk_placement_group_id=disk_placement_group_id,
        )
        if context_key:
            context = {context_key: {'operation_id': created_disk.operation_id, 'disk_id': created_disk.disk_id}}
        else:
            context = dict()
        self.add_change(Change(f'disk.{created_disk.disk_id}', 'create initiated', context=context))
        return created_disk.operation_id, created_disk.disk_id

    def get_instance_disks(self, fqdn, instance_id):
        """
        Get disks attached to instance
        """
        disks_from_context = self.context_get(f'{instance_id}.disks')
        if disks_from_context:
            self.add_change(Change(f'{fqdn}.disks_listed', True, rollback=Change.noop_rollback))
            return {
                'boot': DiskModel.from_json(disks_from_context['boot']),
                'secondary': [DiskModel.from_json(x) for x in disks_from_context['secondary']],
            }
        instance = self.get_instance(fqdn, instance_id)
        if not instance:
            raise ComputeApiError('Unable to find instance {fqdn}'.format(fqdn=fqdn))

        ret = {
            'boot': self._get_disk(instance.boot_disk.disk_id),
            'secondary': [self._get_disk(x.disk_id) for x in instance.secondary_disks],
        }
        self.add_change(
            Change(
                f'{fqdn}.disks_listed',
                True,
                context={
                    f'{instance_id}.disks': {
                        'boot': ret['boot'].to_json(),
                        'secondary': [x.to_json() for x in ret['secondary']],
                    }
                },
                rollback=Change.noop_rollback,
            )
        )
        return ret

    @tracing.trace('Compute Instance Setup Address')
    def get_instance_setup_address(self, fqdn):
        """
        Get address for managed fqdn
        """
        tracing.set_tag('cluster.host.fqdn', fqdn)

        cached = self._address_cache.get(fqdn)
        if cached:
            return cached
        instance = self.get_instance(fqdn)
        if not instance:
            raise ComputeApiError('Instance {fqdn} not found while ' 'getting setup address.'.format(fqdn=fqdn))
        if len(instance.network_interfaces) == 1:
            return self.extract_setup_address(fqdn, instance.network_interfaces[0])
        subnet_id = self._get_geo_subnet(
            self.config.compute.folder_id, self.config.compute.managed_network_id, instance.zone_id
        ).subnet_id
        for interface in reversed(instance.network_interfaces):
            if interface.subnet_id == subnet_id:
                return self.extract_setup_address(fqdn, interface)
        raise ComputeApiError('Unable to find subnet {id} for instance {fqdn}'.format(id=subnet_id, fqdn=fqdn))

    def extract_setup_address(self, fqdn, interface: NetworkInterface):
        """
        Get ipv6 address from interface
        """
        if not interface.primary_v6_address:
            raise ComputeApiError('Unable to get setup address for {fqdn}'.format(fqdn=fqdn))
        address = interface.primary_v6_address.address
        self.logger.info('Using %s as setup address for %s', address, fqdn)
        self._address_cache[fqdn] = address
        return address

    @tracing.trace('Compute Instance Get Public Address')
    def get_instance_public_addresses(self, fqdn):
        """
        Get public ips of instance
        """
        tracing.set_tag('cluster.host.fqdn', fqdn)

        instance = self.get_instance(fqdn)
        ret = []
        interface = instance.network_interfaces[0]

        if interface.primary_v4_address and interface.primary_v4_address.one_to_one_nat.address:
            ret.append((interface.primary_v4_address.one_to_one_nat.address, 4))

        if interface.primary_v6_address:
            ret.append((interface.primary_v6_address.address, 6))

        return ret

    @retry.on_exception(ComputeApiError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Compute Instance Save Meta')
    def save_instance_meta(self, fqdn):
        """
        Save instance metadata for billing
        """
        tracing.set_tag('cluster.host.fqdn', fqdn)

        instance = self.get_instance(fqdn)
        if not instance:
            raise ComputeApiError('Unable to find instance {fqdn}'.format(fqdn=fqdn))
        self.metadb_host.update(fqdn, 'vtype_id', instance.id)
        return instance.id

    @retry.on_exception(ComputeApiError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Compute Disk Placement Group Save Meta')
    def save_disk_placement_group_meta(self, disk_placement_group_id, pg_num):
        """
        Save Disk Placement Group metadata
        """
        tracing.set_tag('cluster.cid.disk_placement_group_id', disk_placement_group_id)

        self.metadb_disk_placement_group.update(disk_placement_group_id, 'COMPLETE', pg_num)

    @retry.on_exception(ComputeApiError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Compute Disk Save Meta')
    def save_disk_meta(self, fqdn, local_id, mount_point):
        """
        Save disk metadata
        """
        tracing.set_tag('cluster.host.fqdn', fqdn)
        instance = self.get_instance(fqdn)
        disks = self.get_instance_disks(fqdn, instance.id)
        target_disk_id = disks['secondary'][0].id

        self.metadb_disk.update(fqdn, local_id, mount_point, 'COMPLETE', target_disk_id, target_disk_id)

    @retry.on_exception(ComputeApiError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Compute Placement Group Save Meta')
    def save_placement_group_meta(self, placement_group_id, fqdn):
        """
        Save Placement Group metadata
        """
        tracing.set_tag('cluster.cid.placement_group_id.fqdn', placement_group_id)

        self.metadb_placement_group.update(placement_group_id, 'COMPLETE', fqdn)

    # pylint: disable=too-many-arguments
    @retry.on_exception(ComputeApiError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Compute Instance Exists')
    def instance_exists(
        self,
        geo: str,
        fqdn: str,
        managed_fqdn: str,
        image_type: str,
        platform_id: str,
        cores: int,
        core_fraction: float,
        memory: int,
        subnet_id: str,
        assign_public_ip: bool,
        security_groups: list[str],
        disk_type_id: str,
        disk_size: int = None,
        root_disk_size: int = None,
        root_disk_type_id: str = 'network-hdd',
        gpus: int = 0,
        folder_id: str = None,
        io_cores: int = 0,
        metadata: str = None,
        service_account_id: str = None,
        revertable: bool = False,
        image_id: str = None,
        labels: dict = None,
        host_group_ids: list[str] = None,
        disk_placement_group_id: str = None,
        vhost_net: bool = False,
        placement_group_id: str = None,
        **create_args,
    ) -> Optional[str]:
        """
        Ensure that instance exists.
        Returns operation id if instance create was initiated
        """
        tracing.set_tag('cluster.host.fqdn', fqdn)

        context_key = f'{fqdn}.instance_create'
        created_context = self.context_get(context_key)
        if created_context:
            if created_context.get('rollback'):
                rollback = self._rollback_fun_from_desc(created_context.get('rollback'))
            else:
                rollback = None
            self.add_change(Change(f'instance.{fqdn}', 'created', rollback=rollback))
            return created_context['id']
        folder_id = folder_id or self.config.compute.folder_id
        instance = self.get_instance(fqdn, folder_id=folder_id)
        not_exists = False
        if not instance:
            not_exists = True
        elif instance.status == InstanceStatus.ERROR:
            self.operation_wait(self.instance_absent(fqdn))
            self.instance_wait(fqdn, self.INSTANCE_DELETED_STATE)
            not_exists = True

        local_disk_chunk_size = None
        if host_group_ids:
            host_group = self.__host_group_client.get_host_group(host_group_ids[0])
            host_type = self.__host_type_client.get_host_type(host_group.type_id)
            local_disk_chunk_size = host_type.disk_size
        if not_exists:
            if image_id:
                image = self._get_image(image_id)
            else:
                image = self._get_latest_image(image_type)
            operation = self._create_instance(
                geo,
                fqdn,
                managed_fqdn,
                image,
                platform_id,
                cores,
                core_fraction,
                gpus,
                io_cores,
                memory,
                subnet_id,
                assign_public_ip,
                security_groups,
                disk_type_id,
                disk_size,
                root_disk_size,
                root_disk_type_id,
                folder_id,
                metadata,
                service_account_id,
                self._get_idempotence_id(f'create.instance.{fqdn}'),
                labels,
                host_group_ids,
                disk_placement_group_id=disk_placement_group_id,
                vhost_net=vhost_net,
                local_disk_chunk_size=local_disk_chunk_size,
                placement_group_id=placement_group_id,
                **create_args,
            )
            context = {'id': operation.operation_id}
            rollback = None
            if revertable:
                context['rollback'] = {'operation': 'instance-delete', 'target': fqdn}
                rollback = self._rollback_fun_from_desc(context['rollback'])
            self.add_change(Change(f'instance.{fqdn}', 'created', {context_key: context}, rollback))
            return operation.operation_id
        return None

    @retry_operations_limit_exceeded
    def ensure_instance_sg_same(self, fqdn, security_groups_ids):
        """
        Updates compute instance's security groups that may be updated without stopping an instance
        """
        tracing.set_tag('cluster.host.fqdn', fqdn)

        context_key = f'{fqdn}.update_instance_network'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(f'instance.{fqdn}', 'security_groups_updated'))
            return operation_from_context['id']

        instance = self.get_instance(fqdn)
        if not instance:
            raise RuntimeError(f'Could not get instance object for host {fqdn}')
        if not len(instance.network_interfaces):
            return
        self.logger.info('Ensure instance security groups %s', fqdn)
        if security_groups_ids is None:
            security_groups_ids = []
        user_subnet = self._vpc_provider.get_subnet(instance.network_interfaces[0].subnet_id)
        user_network = self._vpc_provider.get_network(user_subnet.network_id)
        if not check_security_groups_changes(instance, user_network, security_groups_ids):
            return
        self.logger.info('Should update instance security groups %s', fqdn)
        idempotency_key = self._get_idempotence_id(f'set_security_groups.instance.{instance.id}')
        operation = self.__instances_client.update_instance_network(
            instance.id, idempotency_key=idempotency_key, security_group_ids=security_groups_ids
        )
        context = {'id': operation.operation_id, 'sg_ids': security_groups_ids}
        self.add_change(Change(f'instance.{fqdn}', 'security_groups_updated', {context_key: context}))
        return operation.operation_id

    def set_instance_resources(
        self,
        instance_id,
        platform_id,
        cores,
        core_fraction,
        io_cores,
        memory,
        gpus=None,
        referrer_id: str = MDB_CMS_REFERRER_ID,
        placement_group_id: str = None,
    ):
        """
        Set instance resources (instance will be restarted)
        """
        return self._update_instance(
            instance_id,
            platform_id=platform_id,
            cores=cores,
            core_fraction=core_fraction,
            io_cores=io_cores,
            memory=memory,
            gpus=gpus,
            referrer_id=referrer_id,
            placement_group_id=placement_group_id,
        )

    def update_instance_boot_disk(self, instance_id: str, boot_disk_id: str) -> str:
        return self._update_instance(instance_id, boot_disk_id=boot_disk_id)

    @retry_operations_limit_exceeded
    def _update_instance(
        self,
        instance_id: str,
        platform_id: str = None,
        cores: int = None,
        core_fraction: int = None,
        io_cores: int = None,
        memory: int = None,
        boot_disk_id: str = None,
        gpus: int = None,
        referrer_id: str = MDB_CMS_REFERRER_ID,
        placement_group_id: str = None,
    ) -> str:
        """
        Update instance.

        Instance will be restarted.
        """
        operation_from_context = self.context_get(f'{instance_id}.resources_change')
        if operation_from_context:
            self.add_change(Change(f'instance.{instance_id}', 'resources change initiated'))
            return operation_from_context

        operation = self.__instances_client.update_instance(
            instance_id=instance_id,
            idempotency_key=self._get_idempotence_id(f'set_resources.instance.{instance_id}'),
            cores=int(cores) if cores is not None else None,
            core_fraction=int(core_fraction) if core_fraction is not None else None,
            io_cores=io_cores,
            memory=int(memory) if memory is not None else None,
            platform_id=platform_id,
            disk_id=boot_disk_id,
            gpus=gpus,
            referrer_id=referrer_id,
            placement_group_id=placement_group_id,
        )
        self.add_change(
            Change(
                f'instance.{instance_id}',
                'resources change initiated',
                context={f'{instance_id}.resources_change': operation.operation_id},
            )
        )
        return operation.operation_id

    @retry_operations_limit_exceeded
    def update_instance_attributes(
        self,
        fqdn: Optional[str] = None,
        labels: Optional[dict] = None,
        metadata: Optional[dict] = None,
        service_account_id: Optional[str] = None,
        referrer_id: Optional[str] = MDB_CMS_REFERRER_ID,
        instance_id: Optional[str] = None,
    ):
        """
        Updates compute instance's attributes that may be updated without stopping an instance
        """

        if not (fqdn or instance_id):
            raise RuntimeError('Either fqdn or instance id must be specified')

        tracing.set_tag('cluster.host.fqdn', fqdn)

        operation_from_context = self.context_get(f'{fqdn}.update_instance_attributes')
        if operation_from_context:
            self.add_change(Change(f'instance.{fqdn}', 'updated'))
            return operation_from_context

        instance = self.get_instance(fqdn=fqdn, instance_id=instance_id)
        if not instance:
            raise RuntimeError(f'Could not get instance object for host {fqdn} or instance id {instance_id}')

        operation = self.__instances_client.update_instance(
            instance_id=instance.id,
            idempotency_key=self._get_idempotence_id(f'{fqdn}.update_instance_attributes'),
            labels=labels,
            metadata=metadata,
            service_account_id=service_account_id,
            referrer_id=referrer_id,
        )

        self.add_change(
            Change(
                f'instance.{fqdn}', 'updated', context={f'{fqdn}.update_instance_attributes': operation.operation_id}
            )
        )
        return operation.operation_id

    def get_instance_changes(
        self,
        fqdn,
        instance_id,
        platform_id,
        cores,
        core_fraction,
        gpus,
        io_cores,
        memory,
        disk_type_id,
        disk_size,
        service_account_id,
        security_group_ids,
        root_disk_size=None,
        labels=None,
        metadata=None,
    ):
        """
        Get types of resource changes (resources, nbs_size_down, nbs_size_up, nvme_size, disk_type, state)
        """
        changes_from_context = self.context_get(f'{fqdn}.compute_changes')
        if changes_from_context is not None:
            self.add_change(
                Change(f'{fqdn}.compute_changes', list(changes_from_context), rollback=Change.noop_rollback)
            )
            return set(changes_from_context)
        instance = self.get_instance(fqdn, instance_id)
        if not instance:
            raise ComputeApiError('Unable to find instance "{id}" for host {fqdn}'.format(fqdn=fqdn, id=instance_id))

        changes = set()
        if check_resources_changes(instance, platform_id, cores, core_fraction, gpus, io_cores, memory):
            changes.add('resources')

        user_subnet = self._vpc_provider.get_subnet(instance.network_interfaces[0].subnet_id)
        user_network = self._vpc_provider.get_network(user_subnet.network_id)
        if check_security_groups_changes(instance, user_network, security_group_ids):
            changes.add('instance_network')

        if service_account_id and instance.service_account_id != service_account_id:
            changes.add('instance_attribute')

        if labels is not None and instance.labels != labels:
            changes.add('instance_attribute')

        if metadata is not None and instance.metadata != metadata:
            changes.add('instance_attribute')

        if disk_size and root_disk_size:
            raise RuntimeError('only one of disk_size or root_disk_size should be given')
        if disk_size:
            # Secondary (data) disk modification for managed database
            nbs_disks = instance.secondary_disks
            if not nbs_disks and disk_type_id.startswith('network-'):
                changes.add('disk_type')
            elif nbs_disks and disk_type_id.startswith('local-'):
                changes.add('disk_type')

            if 'disk_type' not in changes:
                if nbs_disks:
                    current_size = self._get_disk(nbs_disks[0].disk_id).size
                    if disk_size > current_size:
                        changes.add('nbs_size_up')
                    elif disk_size < current_size:
                        changes.add('nbs_size_down')
                else:
                    current_size = instance.resources.nvme_disks * nvme_chunk_size(PLATFORM_MAP, instance.platform_id)
                    if disk_size != current_size:
                        changes.add('nvme_size')
        else:
            # Root disk modification for dataproc
            current_size = self._get_disk(instance.boot_disk.disk_id).size
            if root_disk_size > current_size:
                changes.add('nbs_size_up')
            elif root_disk_size < current_size:
                changes.add('nbs_size_down')

        if instance.status != InstanceStatus.RUNNING:
            changes.add('state')

        self.add_change(
            Change(
                f'{fqdn}.compute_changes',
                list(changes),
                context={f'{fqdn}.compute_changes': list(changes)},
                rollback=Change.noop_rollback,
            )
        )
        return changes

    def instance_running(
        self,
        fqdn: Optional[str],
        instance_id: Optional[str],
        context_suffix='start',
        referrer_id: str = MDB_CMS_REFERRER_ID,
    ) -> Optional[str]:
        """
        Ensure instance acquired RUNNING state if possible or throw error.

        Returns operation id if instance start was initiated.
        """
        instance = self.get_instance(fqdn, instance_id=instance_id)
        if not instance:
            raise ComputeApiError('Unable to find instance {fqdn} with id {id}'.format(fqdn=fqdn, id=instance_id))
        context_key = f'start.instance.{instance.id}.{context_suffix}'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(f'instance.{instance.id}', 'start initiated'))
            return operation_from_context
        if instance.status in (InstanceStatus.STOPPED, InstanceStatus.CRASHED):
            operation_id = self._start_instance(
                instance_id=instance.id,
                idempotence_id=self._get_idempotence_id(context_key),
                referrer_id=referrer_id,
            ).operation_id
            self.add_change(Change(f'instance.{instance.id}', 'start initiated', context={context_key: operation_id}))
            return operation_id
        if instance.status in (InstanceStatus.STARTING, InstanceStatus.RESTARTING, InstanceStatus.UPDATING):
            self.instance_wait(fqdn)
        if instance.status == InstanceStatus.RUNNING:
            return None
        raise ComputeApiError(
            'Unexpected instance {fqdn} status for start: {status}'.format(fqdn=fqdn, status=instance.status.name)
        )

    def instance_stopped(
        self,
        fqdn: Optional[str],
        instance_id: Optional[str],
        termination_grace_period: Optional[int] = None,
        context_suffix='stop',
        referrer_id: str = MDB_CMS_REFERRER_ID,
    ) -> Optional[str]:
        """
        Ensure that instance is in stopped state.
        Returns operation id if instance stop was initiated

        It is opposite to instance started and to instance creation (after instance is created it is RUNNING).
        """
        instance = self.get_instance(fqdn, instance_id=instance_id)
        if not instance:
            raise ComputeApiError('Unable to find instance {fqdn} with id {id}'.format(fqdn=fqdn, id=instance_id))
        if instance.status == InstanceStatus.STOPPED:
            self.logger.info('Instance %s already stopped', instance.id)
            return None
        context_key = f'stop.instance.{instance.fqdn}.{context_suffix}'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(f'instance.{instance.id}', 'stop initiated'))
            return operation_from_context
        if instance.status == InstanceStatus.RUNNING:
            operation_id = self._stop_instance(
                instance_id=instance.id,
                termination_grace_period=termination_grace_period,
                idempotence_id=self._get_idempotence_id(f'stop.instance.{instance.id}.{context_suffix}'),
                referrer_id=referrer_id,
            ).operation_id
            self.add_change(
                Change(
                    f'instance.{instance.id}',
                    'stop initiated',
                    rollback=lambda task, safe_revision: self.instance_running(fqdn, instance_id, context_suffix),
                    context={context_key: operation_id},
                ),
            )
            return operation_id
        if instance.status == InstanceStatus.STOPPING:
            self.instance_wait(instance.fqdn, wait_state=InstanceStatus.STOPPED)
            return None
        raise ComputeApiError(
            'Unexpected instance {fqdn} status for stop: {status}'.format(fqdn=fqdn, status=instance.status)
        )

    @tracing.trace('Compute Operation Wait')
    def operation_wait(self, operation_id, timeout=1200, stop_time=None):
        """
        Wait while operation finishes
        """
        if operation_id is None:
            return

        tracing.set_tag('compute.operation.id', operation_id)

        if stop_time is None:
            stop_time = time.time() + timeout
        with self.interruptable:
            while time.time() < stop_time:
                operation = self._get_operation(operation_id)
                if not operation.done:
                    self.logger.info('Waiting for compute operation %s', operation_id)
                    time.sleep(1)
                    continue
                if not operation.error:
                    return
                raise UserExposedComputeApiError(
                    message=operation.error.message, err_type=operation.error.err_type, code=operation.error.code
                )

            msg = '{timeout}s passed. Compute operation {id} is still running'
            raise ComputeApiError(msg.format(timeout=timeout, id=operation_id))

    @tracing.trace('Compute Operations Wait')
    def operations_wait(self, operation_ids, timeout=600):
        """
        Wait while all operations finish
        """
        tracing.set_tag('compute.operation.ids', operation_ids)

        stop_time = time.time() + timeout
        for operation_id in operation_ids:
            self.operation_wait(operation_id, stop_time=stop_time)

    @tracing.trace('Compute Finished Operations Wait')
    def wait_for_any_finished_operation(self, operation_ids, timeout=120.0, extra_delay=1.0):
        """
        Get first finished operation
        """
        if not operation_ids:
            return

        tracing.set_tag('compute.operation.ids', operation_ids)

        stop_time = time.time() + timeout
        with self.interruptable:
            while time.time() < stop_time:
                for operation_id in operation_ids:
                    operation = self._get_operation(operation_id)
                    if operation.error:
                        raise UserExposedComputeApiError(
                            message=operation.error.message,
                            err_type=operation.error.err_type,
                            code=operation.error.code,
                        )
                    if operation.done:
                        return operation_id

                time.sleep(extra_delay)

        raise ComputeApiError(f'{timeout} seconds passed. Some compute operations may still be running')

    @tracing.trace('Compute Instance Wait', ignore_active_span=True)
    def instance_wait(self, fqdn, wait_state: InstanceStatus = InstanceStatus.RUNNING, timeout=600) -> None:
        """
        Wait while instance became needed state
        """
        tracing.set_tag('cluster.host.fqdn', fqdn)
        instance = self.get_instance(fqdn)
        wait_done = False
        if self.INSTANCE_DELETED_STATE == wait_state:
            wait_state_name = self.INSTANCE_DELETED_STATE
        else:
            wait_state_name = wait_state.name
        if instance is None:
            if wait_state != self.INSTANCE_DELETED_STATE:
                raise ComputeApiError(
                    'Expected {fqdn} to be {target}. '
                    'But instance does not exist'.format(
                        fqdn=fqdn,
                        target=wait_state,
                    )
                )
            else:
                wait_done = True
                context_key = f'compute.instance.{fqdn}.{wait_state_name}'
        else:
            context_key = f'compute.instance.{instance.id}.{wait_state_name}'
            wait_done = self.context_get(context_key)
        if not wait_done:
            current_state = None
            stop_time = time.time() + timeout
            while current_state != wait_state:
                if time.time() > stop_time:
                    msg = f'{timeout}s passed. Instance {fqdn} is in state {current_state}, expecting {wait_state}'
                    raise ComputeApiError(msg)
                if instance:
                    current_state = instance.status
                elif wait_state != self.INSTANCE_DELETED_STATE:
                    raise ComputeApiError(
                        'Expected {fqdn} to be {target}. '
                        'But instance does not exist'.format(
                            fqdn=fqdn,
                            target=wait_state,
                        )
                    )
                else:
                    break  # deleted instance
                if current_state in (InstanceStatus.CRASHED, InstanceStatus.ERROR):
                    raise ComputeApiError(
                        'Expected {fqdn} to be {target}. '
                        'But it is in {state}'.format(
                            fqdn=fqdn,
                            target=wait_state,
                            state=current_state,
                        )
                    )
                if current_state == wait_state:
                    break
                self.logger.info('Compute instance %s at %s, waiting for %s', fqdn, current_state.name, wait_state.name)
                time.sleep(1)
            wait_done = True
        self.add_change(Change(context_key, 'wait ok', {context_key: wait_done}))
        return None

    def instance_absent(
        self, fqdn, instance_id=None, folder_id=None, referrer_id: str = MDB_CMS_REFERRER_ID
    ) -> Optional[str]:
        """
        Delete instance from compute api if it exists.
        Returns operation id if instance delete was initiated
        """
        folder_id = folder_id or self.config.compute.folder_id
        tracing.set_tag('cloud.folder.id', folder_id)
        context_key = f'{fqdn}.instance_delete'
        created_context = self.context_get(context_key)
        if created_context:
            self.add_change(Change(f'instance.{fqdn}', 'delete'))
            return created_context['operation_id']
        instance = self.get_instance(fqdn, instance_id, folder_id=folder_id)
        if instance:
            operation = self._delete_instance(
                instance_id=instance.id,
                idempotence_id=self._get_idempotence_id(context_key),
                referrer_id=referrer_id,
            )
            if self._address_cache.get(fqdn):
                del self._address_cache[fqdn]
            context = {'operation_id': operation.operation_id}
            self.add_change(Change(f'instance.{fqdn}', 'delete', {context_key: context}))
            return operation.operation_id
        return None

    def set_instance_public_ip(self, fqdn: str, use_public_ip: bool) -> Optional[str]:
        tracing.set_tag('cluster.host.fqdn', fqdn)

        context_key = f'{fqdn}.update_instance_public_ip'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(f'instance.{fqdn}', 'updated'))
            return operation_from_context.operation_id

        instance = self.get_instance(fqdn=fqdn)
        if not instance:
            raise RuntimeError(f'Could not get instance object for host {fqdn}')

        if is_public_ip_enabled(instance) == use_public_ip:
            self.logger.info('Host has the desired state for public IP already')
            return None

        if use_public_ip:
            operation = self.__instances_client.add_one_to_one_nat(
                instance_id=instance.id,
                idempotency_key=self._get_idempotence_id(f'{fqdn}.public_ip_enable'),
            )
        else:
            operation = self.__instances_client.remove_one_to_one_nat(
                instance_id=instance.id,
                idempotency_key=self._get_idempotence_id(f'{fqdn}.public_ip_disable'),
            )
        self.add_change(
            Change(
                f'instance.{fqdn}',
                'updated',
                context={context_key: {'operation_id': operation.operation_id}},
            )
        )
        return operation.operation_id

    def is_public_ip_changed(self, fqdn: str, use_public_ip: bool) -> bool:
        tracing.set_tag('cluster.host.fqdn', fqdn)

        instance = self.get_instance(fqdn=fqdn)
        if not instance:
            raise RuntimeError(f'Could not get instance object for host {fqdn}')

        return is_public_ip_enabled(instance) != bool(use_public_ip)


def is_public_ip_enabled(instance: InstanceModel) -> bool:
    if not len(instance.network_interfaces):
        return False
    interface = instance.network_interfaces[0]
    if interface.primary_v4_address and interface.primary_v4_address.one_to_one_nat.address:
        return True
    return False
