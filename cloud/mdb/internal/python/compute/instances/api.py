from typing import Callable, Dict, Generator, Iterable, List, NamedTuple, Optional, Union
from yandex.cloud.priv.compute.v1 import (
    instance_pb2,
    instance_service_pb2,
    instance_service_pb2_grpc,
)
from yandex.cloud.priv.reference import reference_pb2

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.models import OperationModel
from cloud.mdb.internal.python.compute.pagination import ComputeResponse, paginate
from cloud.mdb.internal.python.grpcutil.retries import client_retry, client_retry_on_read_method
from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from dbaas_common import tracing
from . import exceptions as client_errors
from .models import (
    AttachedDiskRequest,
    InstanceModel,
    InstanceView,
    Metric,
    SubnetRequest,
    Reference,
    DnsRecordSpec,
    InstanceDnsSpec,
)
from .platforms import nvme_chunk_size, PLATFORM_MAP


class InstancesClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


def dns_record_to_spec(rec: DnsRecordSpec) -> instance_service_pb2.DnsRecordSpec:
    fqdn = rec.fqdn
    if not fqdn.endswith('.'):
        fqdn += '.'
    return instance_service_pb2.DnsRecordSpec(
        fqdn=fqdn,
        dns_zone_id=rec.dns_zone_id,
        ptr=rec.ptr,
        ttl=rec.ttl,
    )


class InstancesClient:
    __instance_service = None

    def __init__(
        self,
        config: InstancesClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='InstancesClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @property
    def _instance_service(self) -> ComputeGRPCService:
        if self.__instance_service is None:
            channel = grpcutil.new_grpc_channel(self.config.transport)
            self.__instance_service = ComputeGRPCService(
                self.logger,
                channel,
                instance_service_pb2_grpc.InstanceServiceStub,
                timeout=self.config.timeout,
                get_token=self.token_getter,
                error_handlers=self.error_handlers,
            )
        return self.__instance_service

    def list_instances(self, folder_id: str, name: str = None) -> Generator[InstanceModel, None, None]:
        """
        Get instances in folder
        """
        request = instance_service_pb2.ListInstancesRequest()
        request.folder_id = folder_id
        request.page_size = self.config.page_size
        if name is not None:
            request.filter = f'name="{name}"'
        return paginate(self._list_instances, request)

    @client_retry_on_read_method
    @tracing.trace('Compute List Instances page')
    def _list_instances(self, request: instance_service_pb2.ListInstancesRequest) -> Iterable:
        tracing.set_tag('compute.folder.id', request.folder_id)
        response = self._instance_service.List(request)
        return ComputeResponse(
            resources=map(InstanceModel.from_api, response.instances),
            next_page_token=response.next_page_token,
        )

    @client_retry
    @tracing.trace('Compute Get Instance')
    def get_instance(self, instance_id: str = None, view: InstanceView = InstanceView.BASIC) -> InstanceModel:
        """
        Get instance info
        """
        tracing.set_tag('compute.instance.id', instance_id)
        request = instance_service_pb2.GetInstanceRequest()
        request.instance_id = instance_id
        request.view = view.value
        return InstanceModel.from_api(self._instance_service.Get(request))

    # pylint: disable=too-many-arguments,too-many-locals
    @client_retry
    @tracing.trace('Compute Instance Create instance')
    def create_instance(
        self,
        zone_id: str,
        fqdn: str,
        boot_disk_size: int,
        image_id: str,
        platform_id: str,
        cores: int,
        core_fraction: int,
        gpus: int,
        io_cores: int,
        memory: int,
        assign_public_ip: bool,
        security_group_ids: List[str],
        disk_type_id: str,
        disk_size: Optional[int],
        boot_disk_type_id: str,
        folder_id: str,
        metadata: dict,
        service_account_id: Optional[str],
        idempotence_id: str,
        name: str,
        secondary_disk_specs: List[AttachedDiskRequest],
        boot_disk_spec: Optional[AttachedDiskRequest],
        subnets: List[SubnetRequest],
        labels: dict,
        references: List[Reference],
        host_group_ids: List[str],
        disk_placement_group_id: str = None,
        simulate_billing: bool = False,
        vhost_net: bool = False,
        local_disk_chunk_size: int = None,
        placement_group_id: str = None,
        dns_spec: InstanceDnsSpec = InstanceDnsSpec(),
    ) -> Union[OperationModel, List[Metric]]:
        tracing.set_tag('compute.instance.fqdn', fqdn)
        request = instance_service_pb2.CreateInstanceRequest(references=self.__get_references_spec(references))
        request.folder_id = folder_id
        request.name = name
        request.fqdn = fqdn
        request.hostname = name
        request.zone_id = zone_id
        request.platform_id = platform_id
        request.resources_spec.memory = int(memory)
        request.resources_spec.cores = int(cores)
        request.resources_spec.core_fraction = int(core_fraction)
        request.resources_spec.gpus = int(gpus)

        if secondary_disk_specs:
            for spec in secondary_disk_specs:
                secondary_disk = request.secondary_disk_specs.add()
                secondary_disk.auto_delete = False
                if secondary_disk.HasField('disk_spec'):
                    raise NotImplementedError('Only explicit disk_id is implemented now')
                secondary_disk.disk_id = spec.disk_id

        if disk_type_id.startswith('local-'):
            if local_disk_chunk_size:
                num_disks = disk_size // local_disk_chunk_size
            else:
                num_disks = disk_size // nvme_chunk_size(PLATFORM_MAP, platform_id)
            if num_disks == 0:
                raise client_errors.ConfigurationError(
                    f'Unexpected local nmve disk size {disk_size} leads to zero disks count'
                )
            request.resources_spec.nvme_disks = num_disks
        elif disk_size is not None and len(request.secondary_disk_specs) == 0:
            secondary_disk = request.secondary_disk_specs.add()
            secondary_disk.auto_delete = True
            secondary_disk.disk_spec.size = int(disk_size)
            secondary_disk.disk_spec.type_id = disk_type_id
            if disk_placement_group_id is not None:
                secondary_disk.disk_spec.disk_placement_policy.placement_group_id = disk_placement_group_id

        if metadata is not None:
            request.metadata.update(metadata)

        if labels is not None:
            request.labels.update(labels)

        if boot_disk_spec:
            request.boot_disk_spec.auto_delete = False
            request.boot_disk_spec.disk_id = boot_disk_spec.disk_id
        else:
            request.boot_disk_spec.auto_delete = True
            request.boot_disk_spec.disk_spec.size = int(boot_disk_size)
            request.boot_disk_spec.disk_spec.image_id = image_id
            request.boot_disk_spec.disk_spec.type_id = boot_disk_type_id

        request.network_interface_specs.extend(
            self.__get_interfaces_spec(
                subnets=subnets,
                assign_public_ip=assign_public_ip,
                security_group_ids=security_group_ids,
                dns_spec=dns_spec,
            )
        )
        if vhost_net:
            request.network_settings.type = instance_pb2.NetworkSettings.Type.STANDARD
            request.network_settings.backend = 'vhost-net'
            request.network_settings.tx_queue_len = 120000

        if io_cores > 0:
            request.network_settings.type = instance_pb2.NetworkSettings.Type.SOFTWARE_ACCELERATED

        if service_account_id is not None:
            request.service_account_id = service_account_id

        if host_group_ids:
            rule = instance_pb2.PlacementPolicy.HostAffinityRule(
                key='yc.hostGroupId',
                op=instance_pb2.PlacementPolicy.HostAffinityRule.Operator.IN,
                values=host_group_ids,
            )
            request.placement_policy.host_affinity_rules.extend([rule])

        if simulate_billing:
            response = self._instance_service.SimulateBillingMetrics(request)
            metrics = []
            for grpc_metric in response.metrics:
                metrics.append(Metric.from_api(grpc_metric))
            return metrics

        if placement_group_id is not None:
            request.placement_policy.placement_group_id = placement_group_id

        operation = self._instance_service.Create(request, idempotency_key=idempotence_id)
        return OperationModel().operation_from_api(
            self.logger, operation, self._instance_service.error_from_rich_status
        )

    def __get_references_spec(self, references: List[Reference]) -> List[reference_pb2.Reference]:
        references_spec = []
        for reference in references:
            reference_spec = reference_pb2.Reference(
                referrer=reference_pb2.Referrer(
                    type=reference.referrer.type.value,
                    id=reference.referrer.id,
                ),
                type=reference_pb2.Reference.Type.MANAGED_BY,
            )
            references_spec.append(reference_spec)
        return references_spec

    def __get_interfaces_spec(
        self,
        subnets: List[SubnetRequest],
        assign_public_ip: bool,
        security_group_ids: Optional[List[str]],
        dns_spec: InstanceDnsSpec,
    ) -> List[instance_service_pb2.NetworkInterfaceSpec]:
        """
        Generate interfaces spec for instance
        """
        result = []
        for index, subnet in enumerate(subnets):
            nis = instance_service_pb2.NetworkInterfaceSpec()
            if subnet.v6_cidr:
                nis.primary_v6_address_spec.SetInParent()
            if subnet.v4_cidr:
                nis.primary_v4_address_spec.SetInParent()
            nis.subnet_id = subnet.subnet_id
            if index == 0:
                if security_group_ids:
                    nis.security_group_ids.extend(security_group_ids)
                if assign_public_ip:
                    if not subnet.v4_cidr:
                        raise client_errors.IPV6OnlyPublicIPError
                    nis.primary_v4_address_spec.one_to_one_nat_spec.ip_version = instance_pb2.IPV4
                if dns_spec.eth0_ip6 and subnet.v6_cidr:
                    nis.primary_v6_address_spec.dns_record_specs.append(dns_record_to_spec(dns_spec.eth0_ip6))
            elif index == 1:
                if dns_spec.eth1_ip6:
                    nis.primary_v6_address_spec.dns_record_specs.append(dns_record_to_spec(dns_spec.eth1_ip6))
            result.append(nis)
        return result

    @client_retry
    @tracing.trace('Compute Instance Start instance')
    def start_instance(self, instance_id: str, idempotency_key: str, referrer_id: str = None) -> OperationModel:
        """
        Start stopped instance
        """
        tracing.set_tag('compute.instance.id', instance_id)
        request = instance_service_pb2.StartInstanceRequest()
        request.instance_id = instance_id
        operation = self._instance_service.Start(request, idempotency_key=idempotency_key, referrer_id=referrer_id)
        return OperationModel().operation_from_api(
            self.logger, operation, self._instance_service.error_from_rich_status
        )

    @client_retry
    @tracing.trace('Compute Instance Stop instance')
    def stop_instance(
        self, instance_id: str, termination_grace_period: Optional[int], idempotency_key: str, referrer_id: str = None
    ) -> OperationModel:
        """
        Stop running instance
        """
        tracing.set_tag('compute.instance.id', instance_id)
        request = instance_service_pb2.StopInstanceRequest()
        request.instance_id = instance_id
        if termination_grace_period is not None:
            request.termination_grace_period.seconds = termination_grace_period
            request.termination_grace_period.nanos = 0
        operation = self._instance_service.Stop(request, idempotency_key=idempotency_key, referrer_id=referrer_id)
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._instance_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('Compute Instance Update metadata')
    def update_metadata(
        self, instance_id: str, metadata: dict, delete_fields: list, idempotency_key: str, referrer_id: str = None
    ) -> OperationModel:
        tracing.set_tag('compute.instance.id', instance_id)
        request = instance_service_pb2.UpdateInstanceMetadataRequest()
        request.instance_id = instance_id
        request.delete.extend(delete_fields)
        request.upsert.update(metadata)
        operation = self._instance_service.UpdateMetadata(
            request,
            idempotency_key=idempotency_key,
            referrer_id=referrer_id,
        )
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._instance_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('Compute Instance Delete instance')
    def delete_instance(self, instance_id: str, idempotency_key: str, referrer_id: str = None) -> OperationModel:
        """
        Delete existing instance
        """
        tracing.set_tag('compute.instance.id', instance_id)
        request = instance_service_pb2.DeleteInstanceRequest()
        request.instance_id = instance_id
        operation = self._instance_service.Delete(
            request,
            idempotency_key=idempotency_key,
            referrer_id=referrer_id,
        )
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._instance_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('Compute Instance Attach disk')
    def attach_disk(self, instance_id: str, disk_id: str, idempotency_key: str) -> OperationModel:
        tracing.set_tag('compute.disk.id', disk_id)
        tracing.set_tag('compute.instance.id', instance_id)
        request = instance_service_pb2.AttachInstanceDiskRequest()
        request.instance_id = instance_id
        request.attached_disk_spec.disk_id = disk_id
        request.attached_disk_spec.auto_delete = True
        request.attached_disk_spec.device_name = disk_id
        operation = self._instance_service.AttachDisk(request, idempotency_key=idempotency_key)
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._instance_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('Compute Instance Detach disk')
    def detach_disk(self, instance_id: str, disk_id: str, idempotency_key: str) -> OperationModel:
        tracing.set_tag('compute.disk.id', disk_id)
        tracing.set_tag('compute.instance.id', instance_id)
        request = instance_service_pb2.DetachInstanceDiskRequest()
        request.instance_id = instance_id
        request.disk_id = disk_id

        operation = self._instance_service.DetachDisk(request, idempotency_key=idempotency_key)
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._instance_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('Compute Instance Set Resources')
    def update_instance(
        self,
        instance_id: str,
        idempotency_key: str,
        disk_id: str = None,
        autodelete_flag: bool = None,
        labels: dict = None,
        metadata: dict = None,
        service_account_id: str = None,
        platform_id: str = None,
        cores: int = None,
        core_fraction: int = None,
        io_cores: int = None,
        memory: int = None,
        gpus: int = None,
        referrer_id: str = None,
        vhost_net: bool = None,
        assign_public_ip: bool = None,
        placement_group_id: str = None,
    ) -> OperationModel:
        tracing.set_tag('compute.instance.id', instance_id)
        request = instance_service_pb2.UpdateInstanceRequest()
        request.instance_id = instance_id
        fields_to_update = set()
        if disk_id is not None:
            tracing.set_tag('compute.disk.id', disk_id)
            request.boot_disk_spec.disk_id = disk_id
            fields_to_update.add('boot_disk_spec')
        if autodelete_flag is not None:
            request.boot_disk_spec.auto_delete = autodelete_flag
            fields_to_update.add('boot_disk_spec')
        if labels is not None:
            request.labels.update(labels)
            fields_to_update.add('labels')
        if metadata is not None:
            request.metadata.update(metadata)
            fields_to_update.add('metadata')
        if service_account_id is not None:
            request.service_account_id = service_account_id
            fields_to_update.add('service_account_id')
        if cores is not None:
            request.resources_spec.cores = cores
            fields_to_update.add('resources_spec')
        if memory is not None:
            request.resources_spec.memory = memory
            fields_to_update.add('resources_spec')
        if core_fraction is not None:
            request.resources_spec.core_fraction = core_fraction
            fields_to_update.add('resources_spec')
        if vhost_net:
            request.network_settings.type = instance_pb2.NetworkSettings.Type.STANDARD
            request.network_settings.backend = 'vhost-net'
            request.network_settings.tx_queue_len = 120000
            fields_to_update.add('network_settings')
        if io_cores is not None:
            if io_cores > 0:
                request.network_settings.type = instance_pb2.NetworkSettings.Type.SOFTWARE_ACCELERATED
            else:
                request.network_settings.type = instance_pb2.NetworkSettings.Type.STANDARD
            fields_to_update.add('network_settings')
        if platform_id is not None:
            request.platform_id = platform_id
            fields_to_update.add('platform_id')
        if gpus is not None:
            request.resources_spec.gpus = gpus
            fields_to_update.add('resources_spec')
        request.update_mask.paths.extend(fields_to_update)

        operation = self._instance_service.Update(request, idempotency_key=idempotency_key, referrer_id=referrer_id)
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._instance_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('Compute Instance Network')
    def update_instance_network(
        self,
        instance_id: str,
        idempotency_key: str,
        interface_index: str = "0",
        security_group_ids: List[str] = None,
    ) -> OperationModel:
        request = instance_service_pb2.UpdateInstanceNetworkInterfaceRequest()
        request.instance_id = instance_id
        request.network_interface_index = interface_index
        fields_to_update = set()
        if security_group_ids is not None:
            fields_to_update.add('security_group_ids')
            request.security_group_ids.extend(security_group_ids)
        request.update_mask.paths.extend(fields_to_update)
        operation = self._instance_service.UpdateNetworkInterface(request, idempotency_key=idempotency_key)
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._instance_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('Compute Instance Add Public IP')
    def add_one_to_one_nat(
        self,
        instance_id: str,
        idempotency_key: str,
        interface_index: str = "0",
    ) -> OperationModel:
        request = instance_service_pb2.AddInstanceOneToOneNatRequest()
        request.instance_id = instance_id
        request.network_interface_index = interface_index
        request.one_to_one_nat_spec.ip_version = instance_pb2.IPV4
        operation = self._instance_service.AddOneToOneNat(request, idempotency_key=idempotency_key)
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._instance_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('Compute Instance Remove Public IP')
    def remove_one_to_one_nat(
        self,
        instance_id: str,
        idempotency_key: str,
        interface_index: str = "0",
    ) -> OperationModel:
        request = instance_service_pb2.RemoveInstanceOneToOneNatRequest()
        request.instance_id = instance_id
        request.network_interface_index = interface_index
        operation = self._instance_service.RemoveOneToOneNat(request, idempotency_key=idempotency_key)
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._instance_service.error_from_rich_status,
        )
