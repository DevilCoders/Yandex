from typing import List

from google.protobuf import field_mask_pb2
from yandex.cloud.priv.vpc.v1 import (
    network_service_pb2,
    network_service_pb2_grpc,
    subnet_service_pb2,
    subnet_service_pb2_grpc,
    route_table_service_pb2,
    route_table_service_pb2_grpc,
)
from yandex.cloud.priv.vpc.v1.inner import (
    address_service_pb2,
    address_service_pb2_grpc,
    fip_bucket_service_pb2,
    fip_bucket_service_pb2_grpc,
    compute_internal_service_pb2,
    compute_internal_service_pb2_grpc,
    network_interface_attachment_service_pb2,
    network_interface_attachment_service_pb2_grpc,
    internal_operation_service_pb2,
    internal_operation_service_pb2_grpc,
)

from yc_common import config, formatting, logging
from yc_common.models import Model
from yc_common.clients.grpc.base import auth_metadata, BaseGrpcClient, get_client, GrpcEndpointConfig
from yc_common.clients.models.grpc.common import GrpcModelMixin, OperationListGrpc, OperationGrpc
from yc_common.clients.models import networks as network_models
from yc_common.fields import StringType

log = logging.get_logger(__name__)


class NetworkClient(BaseGrpcClient):
    service_stub_cls = network_service_pb2_grpc.NetworkServiceStub

    def get(self, id: str) -> network_models.Network:
        request = network_service_pb2.GetNetworkRequest(network_id=id)
        return network_models.Network.from_grpc(self.reading_stub.Get(request))

    def list(self, folder_id, filter=None, page_size=None, page_token=None) -> network_models.NetworkList:
        request = network_service_pb2.ListNetworksRequest(
            folder_id=folder_id,
            filter=filter,
            page_size=page_size,
            page_token=page_token,
        )
        return network_models.NetworkList.from_grpc(self.reading_stub.List(request))

    def create(self, folder_id, name=None, description=None, labels=None) -> network_models.NetworkOperation:
        request = network_service_pb2.CreateNetworkRequest(
            folder_id=folder_id,
            name=name,
            description=description,
            labels=labels,
        )
        return network_models.NetworkOperation.from_grpc(self.writing_stub.Create(request))

    def update(self, network_id, update_mask, name=None, description=None, labels=None) -> network_models.NetworkOperation:
        request = network_service_pb2.UpdateNetworkRequest(
            network_id=network_id,
            update_mask=field_mask_pb2.FieldMask(paths=update_mask),
            name=name,
            description=description,
            labels=labels,
        )
        return network_models.NetworkOperation.from_grpc(self.writing_stub.Update(request))

    def delete(self, network_id) -> network_models.NetworkOperation:
        request = network_service_pb2.DeleteNetworkRequest(
            network_id=network_id,
        )
        return network_models.NetworkOperation.from_grpc(self.writing_stub.Delete(request))

    def list_subnets(self, network_id, page_size=None, page_token=None) -> network_models.SubnetList:
        request = network_service_pb2.ListNetworkSubnetsRequest(
            network_id=network_id,
            page_size=page_size,
            page_token=page_token,
        )
        return network_models.SubnetList.from_grpc(self.reading_stub.ListSubnets(request))

    def list_operations(self, network_id, page_size=None, page_token=None) -> OperationListGrpc:
        request = network_service_pb2.ListNetworkOperationsRequest(
            network_id=network_id,
            page_size=page_size,
            page_token=page_token,
        )
        return OperationListGrpc.from_grpc(self.reading_stub.ListOperations(request))

    def create_default_network(self, folder_id, name=None) -> network_models.CreateDefaultNetworkOperation:
        request = network_service_pb2.CreateDefaultNetworkRequest(
            folder_id=folder_id,
            name=name,
        )
        return network_models.CreateDefaultNetworkOperation.from_grpc(self.writing_stub.CreateDefaultNetwork(request))

    def create_network_system_route_table(self, network_id) -> network_models.NetworkOperation:
        request = network_service_pb2.CreateNetworkSystemRouteTableRequest(
            network_id=network_id,
        )
        return network_models.NetworkOperation.from_grpc(self.writing_stub.CreateNetworkSystemRouteTable(request))

    def move(self, network_id, destination_folder_id) -> network_models.NetworkOperation:
        request = network_service_pb2.MoveNetworkRequest(
            network_id=network_id,
            destination_folder_id=destination_folder_id,
        )
        return network_models.NetworkOperation.from_grpc(self.writing_stub.Move(request))


class SubnetClient(BaseGrpcClient):
    service_stub_cls = subnet_service_pb2_grpc.SubnetServiceStub

    def get(self, id: str) -> network_models.Subnet:
        request = subnet_service_pb2.GetSubnetRequest(subnet_id=id)
        return network_models.Subnet.from_grpc(self.reading_stub.Get(request))

    def list(self, folder_id, filter=None, page_size=None, page_token=None) -> network_models.SubnetList:
        request = subnet_service_pb2.ListSubnetsRequest(
            folder_id=folder_id,
            filter=filter,
            page_size=page_size,
            page_token=page_token,
        )
        return network_models.SubnetList.from_grpc(self.reading_stub.List(request))

    def create(
        self,
        folder_id,
        network_id,
        zone_id,
        name=None,
        description=None,
        labels=None,
        v4_cidr_blocks=(),
        v6_cidr_blocks=(),
        route_table_id=None,
        egress_nat_enable=False,
        extra_params=None,
    ) -> network_models.SubnetOperation:
        request = subnet_service_pb2.CreateSubnetRequest(
            folder_id=folder_id,
            network_id=network_id,
            zone_id=zone_id,
            name=name,
            description=description,
            labels=labels,
            v4_cidr_blocks=v4_cidr_blocks,
            v6_cidr_blocks=v6_cidr_blocks,
            route_table_id=route_table_id,
            egress_nat_enable=egress_nat_enable,
            extra_params=formatting.camelcase_to_underscore(extra_params),
        )
        return network_models.SubnetOperation.from_grpc(self.writing_stub.Create(request))

    def update(
        self,
        subnet_id,
        update_mask,
        name=None,
        description=None,
        labels=None,
        route_table_id=None,
        egress_nat_enable=None,
    ) -> network_models.SubnetOperation:
        request = subnet_service_pb2.UpdateSubnetRequest(
            subnet_id=subnet_id,
            update_mask=field_mask_pb2.FieldMask(paths=update_mask),
            name=name,
            description=description,
            labels=labels,
            route_table_id=route_table_id,
            egress_nat_enable=egress_nat_enable,
        )
        return network_models.SubnetOperation.from_grpc(self.writing_stub.Update(request))

    def delete(self, subnet_id) -> network_models.SubnetOperation:
        request = subnet_service_pb2.DeleteSubnetRequest(
            subnet_id=subnet_id,
        )
        return network_models.SubnetOperation.from_grpc(self.writing_stub.Delete(request))

    def move(self, subnet_id, destination_folder_id) -> network_models.SubnetOperation:
        request = subnet_service_pb2.MoveSubnetRequest(
            subnet_id=subnet_id,
            destination_folder_id=destination_folder_id,
        )
        return network_models.SubnetOperation.from_grpc(self.writing_stub.Move(request))

    def list_operations(self, subnet_id, page_size=None, page_token=None) -> OperationListGrpc:
        request = subnet_service_pb2.ListSubnetOperationsRequest(
            subnet_id=subnet_id,
            page_size=page_size,
            page_token=page_token,
        )
        return OperationListGrpc.from_grpc(self.reading_stub.ListOperations(request))


class RouteTableClient(BaseGrpcClient):
    service_stub_cls = route_table_service_pb2_grpc.RouteTableServiceStub

    def get(self, id: str) -> network_models.RouteTable:
        request = route_table_service_pb2.GetRouteTableRequest(route_table_id=id)
        return network_models.RouteTable.from_grpc(self.reading_stub.Get(request))

    def list(self, folder_id, filter=None, page_size=None, page_token=None) -> network_models.RouteTableList:
        request = route_table_service_pb2.ListRouteTablesRequest(
            folder_id=folder_id,
            filter=filter,
            page_size=page_size,
            page_token=page_token,
        )
        return network_models.RouteTableList.from_grpc(self.reading_stub.List(request))

    def create(
        self,
        folder_id,
        name=None,
        description=None,
        labels=None,
        network_id=None,
        static_routes=(),
    ) -> network_models.RouteTableOperation:
        request = route_table_service_pb2.CreateRouteTableRequest(
            folder_id=folder_id,
            name=name,
            description=description,
            labels=labels,
            network_id=network_id,
            static_routes=static_routes,
        )
        return network_models.RouteTableOperation.from_grpc(self.writing_stub.Create(request))

    def update(
        self,
        route_table_id,
        update_mask,
        name=None,
        description=None,
        labels=None,
        static_routes=(),
    ) -> network_models.RouteTableOperation:
        request = route_table_service_pb2.UpdateRouteTableRequest(
            route_table_id=route_table_id,
            update_mask=field_mask_pb2.FieldMask(paths=update_mask),
            name=name,
            description=description,
            labels=labels,
            static_routes=static_routes,
        )
        return network_models.RouteTableOperation.from_grpc(self.writing_stub.Update(request))

    def delete(self, route_table_id) -> network_models.RouteTableOperation:
        request = route_table_service_pb2.DeleteRouteTableRequest(route_table_id=route_table_id)
        return network_models.RouteTableOperation.from_grpc(self.writing_stub.Delete(request))

    def update_static_routes(self, route_table_id, delete=(), upsert=()) -> network_models.RouteTableOperation:
        request = route_table_service_pb2.UpdateStaticRoutesRequest(
            route_table_id=route_table_id,
            delete=delete,
            upsert=upsert,
        )
        return network_models.RouteTableOperation.from_grpc(self.writing_stub.UpdateStaticRoutes(request))

    def move(self, route_table_id, destination_folder_id) -> network_models.RouteTableOperation:
        request = route_table_service_pb2.MoveRouteTableRequest(
            route_table_id=route_table_id,
            destination_folder_id=destination_folder_id,
        )
        return network_models.RouteTableOperation.from_grpc(self.writing_stub.Move(request))

    def list_operations(self, route_table_id, page_size=None, page_token=None) -> OperationListGrpc:
        request = route_table_service_pb2.ListRouteTableOperationsRequest(
            route_table_id=route_table_id,
            page_size=page_size,
            page_token=page_token,
        )
        return OperationListGrpc.from_grpc(self.reading_stub.ListOperations(request))


class AddressClient(BaseGrpcClient):
    service_stub_cls = address_service_pb2_grpc.AddressServiceStub

    def get(self, id: str) -> network_models.Address:
        request = address_service_pb2.GetAddressRequest(address_id=id)
        return network_models.Address.from_grpc(self.reading_stub.Get(request))

    def list(self, folder_id, filter=None, page_size=None, page_token=None) -> network_models.AddressList:
        request = address_service_pb2.ListAddressesRequest(
            folder_id=folder_id,
            filter=filter,
            page_size=page_size,
            page_token=page_token,
        )
        return network_models.AddressList.from_grpc(self.reading_stub.List(request))

    def create(
        self,
        folder_id,
        name=None,
        description=None,
        labels=None,
        external_address_spec=None,
        internal_address_spec=None,
        ephemeral=False,
    ):
        request = address_service_pb2.CreateAddressRequest(
            folder_id=folder_id,
            name=name,
            description=description,
            labels=labels,
            external_address_spec=external_address_spec,
            internal_address_spec=internal_address_spec,
            ephemeral=ephemeral,
        )
        return network_models.AddressOperation.from_grpc(self.writing_stub.Create(request))

    def update(
        self,
        address_id,
        update_mask,
        name=None,
        description=None,
        labels=None,
        ephemeral=None,
    ):
        request = address_service_pb2.UpdateAddressRequest(
            address_id=address_id,
            update_mask=field_mask_pb2.FieldMask(paths=update_mask),
            name=name,
            description=description,
            labels=labels,
            ephemeral=ephemeral,
        )
        return network_models.AddressOperation.from_grpc(self.writing_stub.Update(request))

    def delete(self, address_id) -> network_models.AddressOperation:
        request = address_service_pb2.DeleteAddressRequest(address_id=address_id)
        return network_models.AddressOperation.from_grpc(self.writing_stub.Delete(request))

    def list_operations(self, address_id, page_size=None, page_token=None) -> OperationListGrpc:
        request = address_service_pb2.ListAddressOperationsRequest(
            address_id=address_id,
            page_size=page_size,
            page_token=page_token,
        )
        return OperationListGrpc.from_grpc(self.reading_stub.ListOperations(request))


class FipBucketClient(BaseGrpcClient):
    service_stub_cls = fip_bucket_service_pb2_grpc.FipBucketServiceStub

    def get(self, bucket_id: str) -> network_models.FipBucket:
        request = fip_bucket_service_pb2.GetFipBucketRequest(bucket_id=bucket_id)
        return network_models.FipBucket.from_grpc(self.reading_stub.Get(request))

    def list(self, folder_id, filter=None, page_size=None, page_token=None) -> network_models.FipBucketList:
        request = fip_bucket_service_pb2.ListFipBucketsRequest(
            folder_id=folder_id,
            filter=filter,
            page_size=page_size,
            page_token=page_token,
        )
        return network_models.FipBucketList.from_grpc(self.reading_stub.List(request))

    def create(
        self,
        folder_id,
        name=None,
        description=None,
        labels=None,
        flavor=None,
        scope=None,
        cidrs=None,
        import_rts=None,
        export_rts=None,
        ip_version=None,
    ) -> network_models.FipBucketOperation:
        request = fip_bucket_service_pb2.CreateFipBucketRequest(
            folder_id=folder_id,
            name=name,
            description=description,
            labels=labels,
            flavor=flavor,
            scope=scope,
            cidrs=cidrs,
            import_rts=import_rts,
            export_rts=export_rts,
            ip_version=ip_version,
        )
        return network_models.FipBucketOperation.from_grpc(self.writing_stub.Create(request))

    def delete(self, bucket_id) -> network_models.FipBucketOperation:
        request = fip_bucket_service_pb2.DeleteFipBucketRequest(bucket_id=bucket_id)
        return network_models.FipBucketOperation.from_grpc(self.writing_stub.Delete(request))

    def add_cidrs(self, bucket_id, cidrs) -> network_models.FipBucketOperation:
        request = fip_bucket_service_pb2.AddFipBucketCidrsRequest(bucket_id=bucket_id, cidrs=cidrs)
        return network_models.FipBucketOperation.from_grpc(self.writing_stub.AddFipBucketCidrs(request))

    def remove_cidrs(self, bucket_id, cidrs) -> network_models.FipBucketOperation:
        request = fip_bucket_service_pb2.RemoveFipBucketCidrsRequest(bucket_id=bucket_id, cidrs=cidrs)
        return network_models.FipBucketOperation.from_grpc(self.writing_stub.RemoveFipBucketCidrs(request))


# Client for java ComputeInternalService.
class VpcComputeInternalClient(BaseGrpcClient):
    service_stub_cls = compute_internal_service_pb2_grpc.ComputeInternalServiceStub

    class AvailabilityTaskRequest(Model, GrpcModelMixin):
        target_id = StringType(required=True)
        update_type = StringType(required=True)
        zone_id = StringType(required=True)

        def get_grpc_class(self):
            return compute_internal_service_pb2.AvailabilityTaskRequest

    def schedule_availability_task(self, requests: List[AvailabilityTaskRequest]):
        request = compute_internal_service_pb2.ScheduleAvailabilityTaskRequest(
            requests=[request.to_grpc() for request in requests],
        )
        self.writing_stub.ScheduleAvailabilityTask(request)


# Client for java NetworkInterfaceAttachmentService.
class VpcNetworkInterfaceAttachmentClient(BaseGrpcClient):
    service_stub_cls = network_interface_attachment_service_pb2_grpc.NetworkInterfaceAttachmentServiceStub

    class InstanceIdRequest(Model, GrpcModelMixin):
        instance_id = StringType(required=True)
        folder_id = StringType(required=True)

        @classmethod
        def get_grpc_class(cls):
            return network_interface_attachment_service_pb2.InstanceIdRequest

    def get(self, instance_id_requests: List[InstanceIdRequest]) -> List[network_models.NetworkInterfaceAttachment]:
        request = network_interface_attachment_service_pb2.GetNetworkInterfaceAttachmentsRequest(
            instance_id_request=[request.to_grpc() for request in instance_id_requests]
        )
        return network_models.GetNetworkInterfaceAttachmentResponse.from_grpc(self.reading_stub.Get(request)).network_interface_attachments

    def authorize_and_validate_create_instance(self, folder_id: str, cloud_id: str, zone_id: str,
                                               network_interfaces: List[network_models.NetworkInterfaceSchema],
                                               dry_run: bool = False, read_only_check: bool = False) -> network_models.ComputeAuthzRequestList:
        request = network_interface_attachment_service_pb2.AuthorizeAndValidateCreateInstanceRequest(
            folder_id=folder_id,
            cloud_id=cloud_id,
            zone_id=zone_id,
            network_interfaces=[network_interface.to_grpc() for network_interface in network_interfaces],
            dry_run=dry_run,
            read_only_check=read_only_check
        )
        return network_models.ComputeAuthzRequestList.from_grpc(self.writing_stub.AuthorizeAndValidateCreateInstance(request))

    def create(self, instance_network_context: network_models.InstanceNetworkContext, create_instance_request_id: str) -> network_models.NetworkInterfaceAttachmentOperation:
        request = network_interface_attachment_service_pb2.CreateNetworkInterfaceAttachmentsRequest(
            instance_network_context=instance_network_context.to_grpc(),
            create_instance_request_id=create_instance_request_id
        )
        return network_models.NetworkInterfaceAttachmentOperation.from_grpc(self.writing_stub.Create(request))

    def allocate(self, instance_network_context: network_models.InstanceNetworkContext) -> network_models.NetworkInterfaceAttachmentOperation:
        request = network_interface_attachment_service_pb2.AllocateNetworkInterfaceAttachmentsRequest(
            instance_network_context=instance_network_context.to_grpc()
        )
        return network_models.NetworkInterfaceAttachmentOperation.from_grpc(self.writing_stub.Allocate(request))

    def deallocate(self, instance_network_context: network_models.InstanceNetworkContext, keep_dynamic_network_resources: bool) -> network_models.NetworkInterfaceAttachmentOperation:
        request = network_interface_attachment_service_pb2.DeallocateNetworkInterfaceAttachmentsRequest(
            instance_network_context=instance_network_context.to_grpc(),
            keep_dynamic_network_resources=keep_dynamic_network_resources
        )
        return network_models.NetworkInterfaceAttachmentOperation.from_grpc(self.writing_stub.Deallocate(request))

    def delete(self, instance_network_context: network_models.InstanceNetworkContext) -> network_models.NetworkInterfaceAttachmentOperation:
        request = network_interface_attachment_service_pb2.DeleteNetworkInterfaceAttachmentsRequest(
            instance_network_context=instance_network_context.to_grpc()
        )
        return network_models.NetworkInterfaceAttachmentOperation.from_grpc(self.writing_stub.Delete(request))


class VpcInternalOperationClient(BaseGrpcClient):
    service_stub_cls = internal_operation_service_pb2_grpc.InternalOperationServiceStub

    def get(self, operation_id: str):
        request = internal_operation_service_pb2.GetInternalOperationRequest(
            operation_id=operation_id
        )
        return OperationGrpc.from_grpc(self.reading_stub.Get(request))


def _get_client(client_cls, iam_token):
    metadata = auth_metadata(iam_token)
    return get_client(
        client_cls,
        metadata=metadata,
        endpoint_config=config.get_value("endpoints.vpc_config_plane", model=GrpcEndpointConfig),
    )


def get_network_client(iam_token) -> NetworkClient:
    return _get_client(NetworkClient, iam_token)


def get_subnet_client(iam_token) -> SubnetClient:
    return _get_client(SubnetClient, iam_token)


def get_route_table_client(iam_token) -> RouteTableClient:
    return _get_client(RouteTableClient, iam_token)


def get_address_client(iam_token) -> AddressClient:
    return _get_client(AddressClient, iam_token)


def get_fip_bucket_client(iam_token) -> FipBucketClient:
    return _get_client(FipBucketClient, iam_token)


def get_vpc_compute_internal_client(iam_token) -> VpcComputeInternalClient:
    return _get_client(VpcComputeInternalClient, iam_token)


def get_vpc_network_interface_attachment_client(iam_token) -> VpcNetworkInterfaceAttachmentClient:
    return _get_client(VpcNetworkInterfaceAttachmentClient, iam_token)


def get_vpc_internal_operation_client(iam_token) -> VpcInternalOperationClient:
    return _get_client(VpcInternalOperationClient, iam_token)
