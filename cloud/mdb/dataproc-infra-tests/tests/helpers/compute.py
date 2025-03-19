"""
YC.Compute interaction helper
"""
import os
import time
import uuid

import grpc

from yandex.cloud.priv.compute.v1 import (
    disk_service_pb2,
    disk_service_pb2_grpc,
    image_pb2,
    image_service_pb2,
    image_service_pb2_grpc,
    instance_pb2,
    instance_service_pb2,
    instance_service_pb2_grpc,
    operation_service_pb2 as compute_operation_service_pb2,
    operation_service_pb2_grpc as compute_operation_service_pb2_grpc,
    snapshot_service_pb2,
    snapshot_service_pb2_grpc,
)
from yandex.cloud.priv.vpc.v1 import (
    network_service_pb2,
    network_service_pb2_grpc,
    operation_service_pb2 as vpc_operation_service_pb2,
    operation_service_pb2_grpc as vpc_operation_service_pb2_grpc,
    subnet_service_pb2,
    subnet_service_pb2_grpc,
    security_group_service_pb2,
    security_group_service_pb2_grpc,
)

from .grpcutil.exceptions import NotFoundError
from .grpcutil.service import WrappedGRPCService


def _get_compute_name_from_fqdn(fqdn):
    """
    Compute does not use fqdn as instance name.
    So we strip domain name from fqdn here
    """
    return fqdn.split('.', maxsplit=1)[0]


def new_grpc_channel(url, cert_file, server_name=None) -> grpc.Channel:
    """
    Initialize gRPC channel using Config
    """
    creds = _get_ssl_creds(cert_file)
    options = None
    if server_name:
        options = (('grpc.ssl_target_name_override', server_name),)
    return grpc.secure_channel(url, creds, options)


def _get_ssl_creds(cert_file):
    if not cert_file or not os.path.exists(cert_file):
        return grpc.ssl_channel_credentials()
    with open(cert_file, 'rb') as file_handler:
        certs = file_handler.read()
    return grpc.ssl_channel_credentials(root_certificates=certs)


class ComputeApiError(Exception):
    """
    Compute interaction error
    """


class VpcApiError(Exception):
    """
    Vpc interaction error
    """


class ComputeApi:
    """
    Compute client
    """

    def __init__(self, config, logger):
        self.config = config
        self.logger = logger
        self._address_cache = dict()
        self._instance_id_cache = dict()
        self._idempotence_ids = dict()
        grpc_timeout = int(self.config['grpc_timeout'])

        channel = new_grpc_channel(self.config['compute_grpc_url'], self.config['ca_path'])
        self.disk_service = WrappedGRPCService(
            self.logger,
            channel,
            disk_service_pb2_grpc.DiskServiceStub,
            timeout=grpc_timeout,
            get_token=lambda: self.config['token'],
        )
        self.image_service = WrappedGRPCService(
            self.logger,
            channel,
            image_service_pb2_grpc.ImageServiceStub,
            timeout=grpc_timeout,
            get_token=lambda: self.config['token'],
        )
        self.instance_service = WrappedGRPCService(
            self.logger,
            channel,
            instance_service_pb2_grpc.InstanceServiceStub,
            timeout=grpc_timeout,
            get_token=lambda: self.config['token'],
        )
        self.compute_operation_service = WrappedGRPCService(
            self.logger,
            channel,
            compute_operation_service_pb2_grpc.OperationServiceStub,
            timeout=grpc_timeout,
            get_token=lambda: self.config['token'],
        )
        self.snapshot_service = WrappedGRPCService(
            self.logger,
            channel,
            snapshot_service_pb2_grpc.SnapshotServiceStub,
            timeout=grpc_timeout,
            get_token=lambda: self.config['token'],
        )

        vpc_channel = new_grpc_channel(self.config['vpc_grpc_url'], self.config['ca_path'])
        self.network_service = WrappedGRPCService(
            self.logger,
            vpc_channel,
            network_service_pb2_grpc.NetworkServiceStub,
            timeout=grpc_timeout,
            get_token=lambda: self.config['token'],
        )
        self.vpc_operation_service = WrappedGRPCService(
            self.logger,
            vpc_channel,
            vpc_operation_service_pb2_grpc.OperationServiceStub,
            timeout=grpc_timeout,
            get_token=lambda: self.config['token'],
        )
        self.subnet_service = WrappedGRPCService(
            self.logger,
            vpc_channel,
            subnet_service_pb2_grpc.SubnetServiceStub,
            timeout=grpc_timeout,
            get_token=lambda: self.config['token'],
        )
        self.security_group_service = WrappedGRPCService(
            self.logger,
            vpc_channel,
            security_group_service_pb2_grpc.SecurityGroupServiceStub,
            timeout=grpc_timeout,
            get_token=lambda: self.config['token'],
        )

    def _get_idempotence_id(self, key):
        """
        Get local idempotence id by key
        """
        if key not in self._idempotence_ids:
            self._idempotence_ids[key] = str(uuid.uuid4())
        return self._idempotence_ids[key]

    @staticmethod
    def paginate(request, method, collection_name):
        request.page_size = 1000

        while True:
            resp = method(request)
            for resource in getattr(resp, collection_name):
                yield resource
            request.page_token = resp.next_page_token
            if not request.page_token:
                break

    def _list_images(self):
        """
        Get images in folder
        """
        request = image_service_pb2.ListImagesRequest(folder_id=self.config['folder_id'])
        return self.paginate(request, self.image_service.List, 'images')

    def _create_image(self, disk_id, name):
        request = image_service_pb2.CreateImageRequest(
            folder_id=self.config['folder_id'],
            name=name,
            disk_id=disk_id,
            description=name,
        )
        return self.image_service.Create(request, idempotency_key=self._get_idempotence_id(f'image.create.{disk_id}'))

    def _delete_image(self, image_id):
        request = image_service_pb2.DeleteImageRequest(image_id=image_id)
        return self.image_service.Delete(request, idempotency_key=self._get_idempotence_id(f'image.delete.{image_id}'))

    def image_exists(self, disk_id, name):
        """
        Create image if not exists
        """
        images = self._list_images()
        for image in images:
            if image.name == name:
                return image.id
        return self._create_image(disk_id, name)

    def image_list(self, prefix=None):
        """
        List images by prefix
        """
        images = self._list_images()
        if not prefix:
            return images
        ret = []
        for image in images:
            if image.name.startswith(prefix):
                ret.append(image)
        return ret

    def image_absent(self, name):
        """
        Remove image if exists
        """
        images = self._list_images()
        for image in images:
            if image.name == name:
                return self._delete_image(image.id)

    def _list_subnets(self, folder_id, network_id):
        """
        Get subnets by folder and network
        """
        request = network_service_pb2.ListNetworkSubnetsRequest(network_id=network_id)
        return self.paginate(request, self.network_service.ListSubnets, 'subnets')

    def _get_subnet(self, subnet_id):
        """
        Get subnet by id
        """
        request = subnet_service_pb2.GetSubnetRequest(subnet_id=subnet_id)
        return self.subnet_service.Get(request)

    def get_geo_subnet(self, folder_id, network_id, zone_id):
        """
        Get first subnet for folder/network/zone combination
        """
        for subnet in self._list_subnets(folder_id, network_id):
            if subnet.zone_id == zone_id:
                return subnet

        raise ComputeApiError(
            'Unable to find subnet for network {network} in {zone}'.format(network=network_id, zone=zone_id)
        )

    def _get_latest_image(self, image_type='postgresql'):
        """
        Get latest (by date) ready image
        """
        latest = None

        search_pattern = 'dbaas-{type}-'.format(type=image_type)
        for image in self._list_images():
            if image.description.startswith(search_pattern):
                if image.status != image_pb2.Image.Status.READY:
                    continue
                if latest is None:
                    latest = image
                elif image.created_at.ToDatetime() > latest.created_at.ToDatetime():
                    latest = image

        if latest is None:
            raise ComputeApiError('Image for \'{type}\' not found'.format(type=image_type))

        return latest

    def list_references(self, instance_id):
        """
        Get instance references
        """
        try:
            request = instance_service_pb2.ListInstanceReferencesRequest(instance_id=instance_id)
            response = self.instance_service.ListReferences(request)
            return response.references
        except NotFoundError as exc:
            self.logger.info('Unable to get instance %s by id: %s', instance_id, repr(exc))

    def get_instance(self, fqdn, instance_id=None, folder_id=None, view='BASIC'):
        """
        Get instance info if exists
        """
        instance_id = instance_id or self._instance_id_cache.get(fqdn)
        if instance_id:
            try:
                proto_view = {
                    'BASIC': instance_service_pb2.InstanceView.BASIC,
                    'FULL': instance_service_pb2.InstanceView.FULL,
                }[view]
                request = instance_service_pb2.GetInstanceRequest(
                    instance_id=instance_id,
                    view=proto_view,
                )
                instance = self.instance_service.Get(request)
            except NotFoundError as exc:
                self.logger.info('Unable to get instance %s by id: %s', fqdn, repr(exc))
            else:
                if fqdn and not self._instance_id_cache.get(fqdn):
                    self._instance_id_cache[fqdn] = instance.id
                return instance

        instances = self.list_instances(folder_id=folder_id or self.config['dataplane_folder_id'])
        for instance in instances:
            if instance.fqdn == fqdn:
                self._instance_id_cache[fqdn] = instance.id
                if view != 'BASIC':
                    return self.get_instance(None, instance_id=instance.id, folder_id=folder_id, view=view)
                return instance

    def get_disk(self, disk_id):
        """
        Get disk by id
        """
        request = disk_service_pb2.GetDiskRequest(disk_id=disk_id)
        return self.disk_service.Get(request)

    def _get_compute_operation(self, operation_id):
        """
        Get compute operation by id
        """
        request = compute_operation_service_pb2.GetOperationRequest(operation_id=str(operation_id))
        return self.compute_operation_service.Get(request)

    def _get_vpc_operation(self, operation_id):
        """
        Get vpc operation by id
        """
        request = vpc_operation_service_pb2.GetOperationRequest(operation_id=str(operation_id))
        return self.vpc_operation_service.Get(request)

    def _get_interfaces_spec(self, subnet_id, geo):
        """
        Generate interfaces spec for instance
        """
        subnets = [
            self._get_subnet(subnet_id),
        ]
        managed_network_id = self.config.get('managed_network_id')
        if managed_network_id:
            subnets.append(
                self.get_geo_subnet(self.config['folder_id'], managed_network_id, self.config['geo_map'].get(geo, geo))
            )

        interfaces = []
        for subnet in subnets:
            nis = instance_service_pb2.NetworkInterfaceSpec()
            if subnet.v6_cidr_blocks:
                nis.primary_v6_address_spec.SetInParent()
            if subnet.v4_cidr_blocks:
                nis.primary_v4_address_spec.SetInParent()
            nis.subnet_id = subnet.id
            interfaces.append(nis)

        return interfaces

    def _delete_instance(self, instance_id, referrer_id=None):
        """
        Delete existing instance
        """
        request = instance_service_pb2.DeleteInstanceRequest(instance_id=instance_id)
        idempotency_key = self._get_idempotence_id(f'instance.delete.{instance_id}')
        return self.instance_service.Delete(request, idempotency_key=idempotency_key, referrer_id=referrer_id)

    def _start_instance(self, instance_id, referrer_id=None):
        """
        Start stopped instance
        """
        request = instance_service_pb2.StartInstanceRequest(instance_id=instance_id)
        idempotency_key = self._get_idempotence_id(f'instance.start.{instance_id}')
        return self.instance_service.Start(request, idempotency_key=idempotency_key, referrer_id=referrer_id)

    def _stop_instance(self, instance_id, referrer_id=None):
        """
        Stop running instance
        """
        request = instance_service_pb2.StopInstanceRequest(instance_id=instance_id)
        idempotency_key = self._get_idempotence_id(f'instance.stop.{instance_id}')
        return self.instance_service.Stop(request, idempotency_key=idempotency_key, referrer_id=referrer_id)

    def instance_exists(
        self,
        geo,
        fqdn,
        image_type,
        platform_id,
        cores,
        core_fraction,
        memory,
        subnet_id,
        root_disk_size,
        root_disk_type_id,
        folder_id=None,
        labels=None,
        metadata=None,
    ):
        folder_id = folder_id or self.config['folder_id']
        instance = self.get_instance(fqdn, folder_id=folder_id)
        create = False
        if not instance:
            create = True
        elif instance.status == instance_pb2.Instance.Status.ERROR:
            delete_operation_id = self.instance_absent(fqdn)
            self.compute_operation_wait(delete_operation_id)
            self.wait_instance_deleted(fqdn)
            create = True

        if create:
            request = instance_service_pb2.CreateInstanceRequest()
            request.folder_id = folder_id
            request.name = _get_compute_name_from_fqdn(fqdn)
            request.fqdn = fqdn
            request.hostname = _get_compute_name_from_fqdn(fqdn)
            request.zone_id = geo
            request.platform_id = platform_id
            if metadata:
                request.metadata.update(metadata)
            if labels:
                request.labels.update(labels)
            request.resources_spec.memory = int(memory)
            request.resources_spec.cores = int(cores)
            request.resources_spec.core_fraction = int(core_fraction)

            image = self._get_latest_image(image_type)
            request.boot_disk_spec.auto_delete = True
            request.boot_disk_spec.disk_spec.size = int(root_disk_size)
            request.boot_disk_spec.disk_spec.image_id = image.id
            request.boot_disk_spec.disk_spec.type_id = root_disk_type_id

            request.network_interface_specs.extend(self._get_interfaces_spec(subnet_id, geo))

            idempotency_key = self._get_idempotence_id(f'instance.create.{fqdn}')
            operation = self.instance_service.Create(request, idempotency_key=idempotency_key)
            return operation.id

    def instance_running(self, fqdn, instance_id, referrer_id=None):
        """
        Ensure that instance is in running state.
        Returns operation id if instance start was initiated
        """
        instance = self.get_instance(fqdn, instance_id=instance_id)
        if not instance:
            raise ComputeApiError('Unable to find instance {fqdn} with id {id}'.format(fqdn=fqdn, id=instance_id))
        if instance.status in (instance_pb2.Instance.Status.STOPPED, instance_pb2.Instance.Status.CRASHED):
            operation = self._start_instance(instance.id, referrer_id=referrer_id)
            return operation.id
        if instance.status == instance_pb2.Instance.Status.RUNNING:
            return
        raise ComputeApiError(
            'Unexpected instance {fqdn} status for start: {status}'.format(fqdn=fqdn, status=instance.status)
        )

    def instance_stopped(self, fqdn, instance_id, referrer_id=None):
        """
        Ensure that instance is in stopped state.
        Returns operation id if instance stop was initiated
        """
        instance = self.get_instance(fqdn, instance_id=instance_id)
        if not instance:
            raise ComputeApiError('Unable to find instance {fqdn} with id {id}'.format(fqdn=fqdn, id=instance_id))
        if instance.status == instance_pb2.Instance.Status.RUNNING:
            operation = self._stop_instance(instance.id, referrer_id=referrer_id)
            return operation.id
        if instance.status == instance_pb2.Instance.Status.STOPPED:
            return
        raise ComputeApiError(
            'Unexpected instance {fqdn} status for stop: {status}'.format(fqdn=fqdn, status=instance.status)
        )

    def compute_operation_wait(self, operation_id, timeout=600):
        """
        Wait while compute operation finishes
        """
        if operation_id is None:
            return
        stop_time = time.time() + timeout
        while time.time() < stop_time:
            operation = self._get_compute_operation(operation_id)
            if not operation.done:
                self.logger.debug('Waiting for compute operation %s', operation_id)
                time.sleep(1)
                continue
            if operation.HasField('error'):
                raise ComputeApiError(f'compute operation {operation_id} failed with error {operation.error}')
            return

        raise ComputeApiError(f'{timeout}s passed. Compute operation {operation_id} is still running')

    def vpc_operation_wait(self, operation_id, timeout=600):
        """
        Wait while vpc operation finishes
        """
        if operation_id is None:
            return
        stop_time = time.time() + timeout
        while time.time() < stop_time:
            operation = self._get_vpc_operation(operation_id)
            if not operation.done:
                self.logger.debug('Waiting for vpc operation %s', operation_id)
                time.sleep(1)
                continue
            if operation.HasField('error'):
                raise VpcApiError(f'vpc operation {operation_id} failed with error {operation.error}')
            return

        raise VpcApiError(f'{timeout}s passed. Vpc operation {operation_id} is still running')

    def wait_instance_deleted(self, fqdn, timeout=600):
        """
        Wait while instance became needed state
        """
        current_state = None
        stop_time = time.time() + timeout
        while time.time() < stop_time:
            instance = self.get_instance(fqdn)
            if not instance:
                return
            current_state = instance.status
            if current_state in (instance_pb2.Instance.Status.CRASHED, instance_pb2.Instance.Status.ERROR):
                raise ComputeApiError(
                    'Expected {fqdn} to be deleted. But it is in {state}'.format(fqdn=fqdn, state=current_state)
                )
            self.logger.info('Compute instance %s: %s', fqdn, current_state)
            time.sleep(1)
        raise ComputeApiError(
            'Timed out waiting for instance {fqdn} to be deleted. It is in {state} state'.format(
                fqdn=fqdn, state=current_state
            )
        )

    def instance_absent(self, fqdn, instance_id=None, folder_id=None, referrer_id=None):
        """
        Delete instance from compute api if it exists.
        Returns operation id if instance delete was initiated
        """
        folder_id = folder_id or self.config['folder_id']
        instance = self.get_instance(fqdn, instance_id, folder_id=folder_id)
        if instance:
            operation = self._delete_instance(instance.id, referrer_id)
            if self._address_cache.get(fqdn):
                del self._address_cache[fqdn]
            return operation.id

    def list_instances(self, folder_id, name=None):
        """
        Get instances in folder
        """
        request = instance_service_pb2.ListInstancesRequest(folder_id=folder_id)
        if name is not None:
            request.filter = f'name="{name}"'
        instances = self.paginate(request, self.instance_service.List, 'instances')
        return instances

    def create_snapshot(self, disk_id, folder_id=None, labels=None):
        request = snapshot_service_pb2.CreateSnapshotRequest(
            folder_id=folder_id or self.config['folder_id'],
            name=f'dataproc-{disk_id}',
            disk_id=disk_id,
            labels=labels,
        )
        idempotency_key = self._get_idempotence_id(f'snapshot.create.{disk_id}')
        operation = self.snapshot_service.Create(request, idempotency_key=idempotency_key)
        unpacked_meta = snapshot_service_pb2.CreateSnapshotMetadata()
        operation.metadata.Unpack(unpacked_meta)
        return operation.id, unpacked_meta.snapshot_id

    def list_snapshots(self, folder_id):
        """
        List snapshots in folder
        """
        request = snapshot_service_pb2.ListSnapshotsRequest(folder_id=folder_id)
        return self.paginate(request, self.snapshot_service.List, 'snapshots')

    def delete_snapshot(self, snapshot_id):
        """
        Delete snapshot
        """
        request = snapshot_service_pb2.DeleteSnapshotRequest(snapshot_id=snapshot_id)
        idempotency_key = self._get_idempotence_id(f'snapshot.delete.{snapshot_id}')
        return self.snapshot_service.Delete(request, idempotency_key=idempotency_key).id

    def list_security_groups(self, folder_id):
        """
        List security_groups in folder
        """
        request = security_group_service_pb2.ListSecurityGroupsRequest(folder_id=folder_id)
        return self.paginate(request, self.security_group_service.List, 'security_groups')

    def delete_security_group(self, security_group_id):
        """
        Delete security_group
        """
        request = security_group_service_pb2.DeleteSecurityGroupRequest(security_group_id=security_group_id)
        idempotency_key = self._get_idempotence_id(f'security_group.delete.{security_group_id}')
        return self.security_group_service.Delete(request, idempotency_key=idempotency_key).id
