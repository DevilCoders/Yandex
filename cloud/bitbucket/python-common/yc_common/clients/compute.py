"""Compute client."""

import requests
import time
import typing

from typing import Dict, Iterable, List

import yc_common.clients.models.base as base_models
import yc_common.clients.models.console as console_models
import yc_common.clients.models.instances as instance_models
import yc_common.clients.models.networks as network_models
import yc_common.clients.models.placement_groups as placement_groups_models
import yc_common.clients.models.snapshots as snapshot_models
import yc_common.clients.models.disks as nbs_models
import yc_common.clients.models.zones as zones_models
import yc_common.clients.models.images as images_models
import yc_common.clients.models.operations as operations_models
import yc_common.clients.models.target_groups as target_groups_models
import yc_common.clients.models.network_load_balancers as network_load_balancers_models
import yc_common.clients.models.solomon as solomon_models
import yc_common.clients.models.commitments as commitment_models

from yc_common import config, paging, constants, validation
from yc_common.clients.models.quotas import Quota, FolderQuota, Quotas
from yc_common.constants import ServiceNames
from yc_common.exceptions import Error, ApiError, GrpcStatus, LogicalError
from yc_common.misc import drop_none
from yc_common.models import Model, StringType
from yc_requests.credentials import YandexCloudCredentials

from .api import ApiClient, YcClientAuthorizationError, YcClientError


_UNSET = object()
_OPERATION_WAIT_TIMEOUT = 30
T = typing.TypeVar("T")


class ComputeErrorCodes:
    PlatformNotFound = "PlatformNotFound"
    InvalidResourcesSpecification = "InvalidResourcesSpecification"

    PlacementGroupNotFound = "PlacementGroupNotFound"
    DuplicatePlacementGroupName = "DuplicatePlacementGroupName"
    PlacementGroupNotEmpty = "PlacementGroupNotEmpty"

    DuplicateInstanceName = "DuplicateInstanceName"
    DuplicateInstanceHostname = "DuplicateInstanceHostname"
    InvalidBlockDeviceMapping = "InvalidBlockDeviceMapping"
    InvalidInstancePlacementPolicy = "InvalidInstancePlacementPolicy"
    InvalidInstanceSchedulingPolicy = "InvalidInstanceSchedulingPolicy"
    NotEnoughResourcesForBlockDeviceMapping = "NotEnoughResourcesForBlockDeviceMapping"
    NotEnoughResources = "NotEnoughResources"
    InstanceRemovalConflict = "InstanceRemovalConflict"

    InstanceNotFound = "InstanceNotFound"
    InvalidInstanceState = "InvalidInstanceState"
    InvalidInstanceComputeNode = "InvalidInstanceComputeNode"
    InstanceIsAlreadyStarted = "InstanceIsAlreadyStarted"
    InstanceNotAssignedToSpecifiedNode = "InstanceNotAssignedToSpecifiedNode"
    InstanceIsNotMigratable = "InstanceIsNotMigratable"
    InstanceMigrationFailed = "InstanceMigrationFailed"
    InstanceDetachDiskRequestConflict = "InstanceDetachDiskRequestConflict"
    InstanceAttachDiskRequestConflict = "InstanceAttachDiskRequestConflict"
    InstanceDetachDiskRequestFailed = "InstanceDetachDiskRequestFailed"
    InstanceCannotBeDeallocated = "InstanceCannotBeDeallocated"

    OperationConflictsWithTasks = "OperationConflictsWithTasks"

    DiskNotFound = "DiskNotFound"
    InvalidDiskState = "InvalidDiskState"
    DuplicateDiskName = "DuplicateDiskName"
    DuplicateDeviceName = "DuplicateDeviceName"
    DiskRemovalConflict = "DiskRemovalConflict"

    DuplicateSnapshotName = "DuplicateSnapshotName"
    InvalidSnapshotState = "InvalidSnapshotState"
    InvalidSnapshotSize = "InvalidSnapshotSize"
    SnapshotNotFound = "SnapshotNotFound"
    SnapshotRemovalConflict = "SnapshotRemovalConflict"

    IdempotenceConflict = "IdempotenceConflict"

    OperationNotFound = "OperationNotFound"
    OperationConflict = "OperationConflict"
    InvalidOperationStatus = "InvalidOperationStatus"
    OperationCancellationIsNotSupported = "OperationCancellationIsNotSupported"
    OperationCancelled = "OperationCancelled"

    ZoneNotFound = "ZoneNotFound"
    DuplicateZoneId = "DuplicateZoneId"
    ZoneIsDown = "ZoneIsDown"

    ImageNotFound = "ImageNotFound"
    InvalidImageState = "InvalidImageState"
    DuplicateImageName = "DuplicateImageName"
    ImageRemovalConflict = "ImageRemovalConflict"

    NetworkNotFound = "NetworkNotFound"
    DuplicateNetworkName = "DuplicateNetworkName"
    NetworkNotEmpty = "NetworkNotEmpty"

    DnsRecordNotFound = "DnsRecordNotFound"

    SubnetNotFound = "SubnetNotFound"
    DuplicateSubnetName = "DuplicateSubnetName"
    OverlappingSubnetCIDRs = "OverlappingSubnetCIDRs"
    InvalidSubnetCIDR = "InvalidSubnetCIDR"
    SubnetNotEmpty = "SubnetNotEmpty"
    SubnetHasRouteTable = "SubnetHasRouteTable"

    AddressNotFound = "AddressNotFound"
    DuplicateAddressName = "DuplicateAddressName"
    AddressInUse = "AddressInUse"
    InvalidAddressType = "InvalidAddressType"
    RequestedAddressNotAvailable = "RequestedAddressNotAvailable"
    AddressSpaceExhausted = "AddressSpaceExhausted"
    EphemeralAddressDeallocated = "EphemeralAddressDeallocated"

    InternalAddressNotFound = "InternalAddressNotFound"
    InternalAddressInUse = "InternalAddressInUse"

    FipBucketNotFound = "FipBucketNotFound"
    FipBucketNotEmpty = "FipBucketNotEmpty"
    DuplicateFipBucketName = "DuplicateFipBucketName"
    OverlappingFipBucketCIDRs = "OverlappingFipBucketCIDRs"
    InvalidFipBucketCIDRs = "InvalidFipBucketCIDRs"

    FolderNotFound = "FolderNotFound"
    CloudNotFound = "CloudNotFound"
    DestinationFolderAnotherCloud = "DestinationFolderAnotherCloud"
    DestinationFolderIsTheSame = "DestinationFolderIsTheSame"

    TargetGroupNotFound = "TargetGroupNotFound"
    TargetGroupTargetNotFound = "TargetGroupTargetNotFound"
    DuplicateTargetGroupName = "DuplicateTargetGroupName"
    TargetGroupTargetAlreadyExists = "TargetGroupTargetAlreadyExists"
    TargetGroupTargetsIntersection = "TargetGroupTargetsIntersection"
    TargetGroupTargetNotInSubnet = "TargetGroupTargetNotInSubnet"
    TargetGroupTargetsMultipleNetworks = "TargetGroupTargetsMultipleNetworks"
    TargetGroupNotFoundInFolder = "TargetGroupNotFoundInFolder"

    NetworkLoadBalancerNotFound = "NetworkLoadBalancerNotFound"
    DuplicateNetworkLoadBalancerName = "DuplicateNetworkLoadBalancerName"
    InvalidNetworkLoadBalancerState = "InvalidNetworkLoadBalancerState"
    NetworkLoadBalancerAlreadyStarted = "NetworkLoadBalancerAlreadyStarted"
    NetworkLoadBalancerAlreadyHasAttachment = "NetworkLoadBalancerAlreadyHasAttachment"
    NetworkLoadBalancerNotFoundInFolder = "NetworkLoadBalancerNotFoundInFolder"
    NetworkLoadBalancerListenerAlreadyExists = "NetworkLoadBalancerListenerAlreadyExists"
    NetworkLoadBalancerListenerNotFound = "NetworkLoadBalancerListenerNotFound"
    DuplicateNetworkLoadBalancerListenerName = "DuplicateNetworkLoadBalancerListenerName"
    DuplicateNetworkLoadBalancerListenerPort = "DuplicateNetworkLoadBalancerListenerPort"
    InvalidNetworkLoadBalancerListenerAddressType = "InvalidNetworkLoadBalancerListenerAddressType"
    InvalidNetworkLoadBalancerAddressCount = "InvalidNetworkLoadBalancerAddressCount"

    RouteTableNotFound = "RouteTableNotFound"
    DuplicateRouteTableName = "DuplicateRouteTableName"
    RouteTableInUse = "RouteTableInUse"
    RouteTableIsSystem = "RouteTableIsSystem"
    OverlappingStaticRouteISubnetCIDR = "OverlappingStaticRouteISubnetCIDR"

    ComputeNodeIsActive = "ComputeNodeIsActive"

    SoftLimitExceeded = "SoftLimitExceeded"

    IdentityAuthorizeInstanceNotFound = "IdentityAuthorizeInstanceNotFound"
    IdentityAuthorizeInstanceNodeServiceAccountFailed = "IdentityAuthorizeInstanceNodeServiceAccountFailed"

    CommitmentNotFound = "CommitmentNotFound"
    DuplicateCommitmentName = "DuplicateCommitmentName"
    InvalidCommitmentResources = "InvalidCommitmentResources"
    InvalidCommitmentPeriod = "InvalidCommitmentPeriod"
    InvalidCommitmentState = "InvalidCommitmentState"

    EgressNatUnavailable = "EgressNatUnavailable"

    InvalidNetworkInterfaceSpecification = "InvalidNetworkInterfaceSpecification"
    OneToOneNatAlreadyExists = "OneToOneNatAlreadyExists"


class InstanceMigrationFailureSimulation:
    ALLOCATION_ACQUIRING_ERROR = "allocation-acquiring-error"
    REJECTED_BY_COMPUTE_NODE = "rejected-by-compute-node"

    OUTGOING_MIGRATION_CONNECTION_ERROR = "outgoing-migration-connection-error"
    OUTGOING_MIGRATION_FAILURE = "outgoing-migration-failure"

    MIGRATING_INSTANCE_ACCEPTION = "migrating-instance-acception"
    INSTANCE_PREPARING = "instance-preparing"
    INCOMING_MIGRATION_FAILURE = "incoming-migration-failure"
    TARGET_PREPARING_HANG = "target-preparing-hang"
    TARGET_HYPERVISOR_HANG_DURING_MIGRATION = "target-hypervisor-hang-during-migration"
    TARGET_HYPERVISOR_CRASH_DURING_MIGRATION = "target-hypervisor-crash-during-migration"

    ALL = [
        ALLOCATION_ACQUIRING_ERROR, REJECTED_BY_COMPUTE_NODE,

        OUTGOING_MIGRATION_CONNECTION_ERROR, OUTGOING_MIGRATION_FAILURE,

        MIGRATING_INSTANCE_ACCEPTION, INSTANCE_PREPARING, INCOMING_MIGRATION_FAILURE,
        TARGET_PREPARING_HANG, TARGET_HYPERVISOR_HANG_DURING_MIGRATION, TARGET_HYPERVISOR_CRASH_DURING_MIGRATION,
    ]


class OperationTimedOutError(Error):
    def __init__(self, operation_id):
        super().__init__("Operation completion awaiting has timed out. id = {}.", operation_id)


class _Api2Client(ApiClient):
    def _parse_error(self, response):
        error, _ = self._parse_json_response(response, model=operations_models.ErrorV1Alpha1)

        if response.status_code == requests.codes.unauthorized:
            raise YcClientAuthorizationError(error.message)
        elif error.type in ("RequestValidationError", "ErrInputValidationFailure"):
            # FIXME: Which error codes we should also ignore?
            raise YcClientError(error.message)
        else:
            raise ApiError(GrpcStatus(error.code), error.type, error.message)


class ComputeClient:
    CURRENT_VERSION = "v1alpha1"
    NEXT_VERSION = "v1"
    STAGING_VERSION = "staging"

    # NOTE: YandexCloudCredentials are deprecated, use iam_token instead
    def __init__(self, url, credentials: YandexCloudCredentials=None, api_version=NEXT_VERSION, iam_token=None):
        # FIXME: Timeout: LBS may respond too long due to global locks even on GET requests
        self.__client = _Api2Client(url, credentials=credentials, service_name=ServiceNames.COMPUTE,
                                    timeout=constants.SERVICE_REQUEST_TIMEOUT, iam_token=iam_token)
        self.__api_version = api_version

    def __api_path(self, path, api_version=None):
        return "/" + (api_version or self.__api_version) + path

    def __call_operation(self, method, path, request=None, params=None,
                         idempotence_id=None, wait_timeout=0, api_version=None, model: typing.Type[T]=None, timeout: int=None) -> T:
        operation = self.__client.call(
            method, self.__api_path(path, api_version=api_version), request=request, params=params,
            extra_headers=drop_none({constants.IDEMPOTENCE_HEADER: idempotence_id}),
            model=model, timeout=timeout,
        )
        return self.wait_operation(operation, wait_timeout=wait_timeout, api_version=api_version, model=model)

    def __wait_operation(self, operation: operations_models.OperationV1Beta1, wait_timeout, **kwargs) -> operations_models.OperationV1Beta1:
        # CLOUD-20436: Parallel invocation of tests may slow up progress
        parallel_mode_timeout_multiplier = config.get_value("parallel_mode_timeout_multiplier", default=1)
        wait_timeout = wait_timeout * parallel_mode_timeout_multiplier
        if wait_timeout == 0 or operation.done:
            return operation

        start_time = time.monotonic()
        while time.monotonic() - start_time < wait_timeout:
            operation = self.get_operation(operation.id, **kwargs)
            if operation.done:
                return operation

            time.sleep(0.1)

        raise OperationTimedOutError(operation.id)

    def __post_operation(self, path, request, model: typing.Type[T]=None, **kwargs) -> T:
        return self.__call_operation("POST", path, request, model=model, **kwargs)

    def __patch_operation(self, path, request, model: typing.Type[T]=None, **kwargs) -> T:
        return self.__call_operation("PATCH", path, request, model=model, **kwargs)

    def __delete_operation(self, path, model: typing.Type[T]=None, **kwargs) -> T:
        return self.__call_operation("DELETE", path, model=model, **kwargs)

    def __get_response(self, path, api_version=None, model: typing.Type[T]=None, **kwargs) -> typing.Union[T, dict]:
        return self.__client.get(self.__api_path(path, api_version=api_version), model=model, **kwargs)

    def __post_response(self, path, api_version=None, request=None, model: typing.Type[T]=None, **kwargs) -> typing.Union[T, dict]:
        return self.__client.post(self.__api_path(path, api_version=api_version), request, model=model, **kwargs)

    def get_operation(self, operation_id, model: typing.Type[T]=operations_models.OperationV1Beta1, **kwargs) -> T:
        return self.__get_response(
            "/operations/" + operation_id,
            model=model,
            **kwargs
        )

    def cancel_operation(self, operation_id, **kwargs) -> operations_models.OperationV1Beta1:
        return self.__post_operation("/operations/" + operation_id + "/cancel", {}, **kwargs)

    def wait_operation(self, operation: operations_models.OperationV1Beta1, wait_timeout=_OPERATION_WAIT_TIMEOUT, check_status=True, **kwargs) -> operations_models.OperationV1Beta1:
        operation = self.__wait_operation(operation, wait_timeout, **kwargs)

        if check_status:
            error = operation.get_error()
            if error is not None:
                raise ApiError(GrpcStatus(error["code"]), error["type"], error["message"], operation_id=operation.id)

        return operation

    # Placement groups

    def iter_placement_groups(self, folder_id, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[placement_groups_models.PlacementGroup]:
        return paging.iter_items(self.list_placement_groups, "placement_groups", page_size, folder_id, name=name)

    def list_placement_groups(self, folder_id, name=None, page_size=None, page_token=None):
        return self.__get_response("/placementGroups", params=drop_none({
            "folderId": folder_id,
            "filter": None if name is None else "name = {!r}".format(name),
            "pageSize": page_size,
            "pageToken": page_token,
        }), model=placement_groups_models.PlacementGroupList)

    def iter_placement_group_instances(
        self, group_id, page_size=base_models.DEFAULT_PAGE_SIZE
    ) -> Iterable[instance_models.InstanceV1Beta1]:
        return paging.iter_items(self.list_placement_group_instances, "instances", page_size, group_id)

    def list_placement_group_instances(
        self, group_id, page_size=None, page_token=None, model=instance_models.InstanceListV1Beta1,
    ) -> instance_models.InstanceListV1Beta1:
        return self.__get_response(
            "/placementGroups/" + group_id + "/instances",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=model,
        )

    def iter_placement_group_operations(self, group_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_placement_group_operations, "operations", page_size, group_id)

    def list_placement_group_operations(self, group_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/placementGroups/" + group_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=operations_models.OperationListV1Beta1
        )

    def get_placement_group(self, group_id) -> placement_groups_models.PlacementGroup:
        return self.__get_response("/placementGroups/" + group_id, model=placement_groups_models.PlacementGroup)

    def create_placement_group(
        self, folder_id, name=None, description=None, labels=None,
        spread_placement_strategy: placement_groups_models.SpreadPlacementStrategy=None, **kwargs
    ):
        return self.__post_operation("/placementGroups", drop_none({
            "folderId": folder_id,
            "name": name,
            "description": description,
            "labels": labels,
            "spreadPlacementStrategy": spread_placement_strategy,
        }), model=placement_groups_models.PlacementGroupOperation, **kwargs)

    def modify_placement_group(
        self, group_id, name=_UNSET, description=_UNSET, labels=_UNSET, spread_placement_strategy=_UNSET, **kwargs
    ):
        update = drop_none({
            "name": name,
            "description": description,
            "labels": labels,
            "spreadPlacementStrategy": spread_placement_strategy,
        }, none=_UNSET)
        if not update:
            raise LogicalError()

        return self.__patch_operation("/placementGroups/" + group_id, dict(
            update, updateMask=",".join(update.keys())
        ), model=placement_groups_models.PlacementGroupOperation, **kwargs)

    def delete_placement_group(self, group_id, **kwargs):
        return self.__delete_operation(
            "/placementGroups/" + group_id, model=placement_groups_models.PlacementGroupOperation, **kwargs)

    # Instances

    def iter_instance_operations(self, instance_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_instance_operations, "operations", page_size, instance_id)

    def list_instance_operations(self, instance_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/instances/" + instance_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=operations_models.OperationListV1Beta1)

    def iter_instances(self, folder_id, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[instance_models.InstanceV1Beta1]:
        return paging.iter_items(self.list_instances, "instances", page_size, folder_id, name=name)

    def list_instances(
        self, folder_id, name=None, page_size=None, page_token=None, model=instance_models.InstanceListV1Beta1,
    ) -> instance_models.InstanceListV1Beta1:
        return self.__get_response(
            "/instances",
            params=drop_none({
                "folderId": folder_id,
                "pageSize": page_size,
                "pageToken": page_token,
                "name": name,
            }),
            model=model,
        )

    def get_instance(self, instance_id, view: instance_models.InstanceView=None, model=instance_models.InstanceV1Beta1) -> instance_models.InstanceV1Beta1:
        return self.__get_response(
            "/instances/" + instance_id,
            params=drop_none({
                "view": view.name if view is not None else None,
            }),
            model=model
        )

    def create_instance(
        self, *,
        folder_id, zone_id, platform_id, resources: instance_models.InstanceResourcesSpec,
        name=None, description=None, labels=None, boot_disk_spec: nbs_models.AttachedDiskSpec=None,
        secondary_disk_specs: List[nbs_models.AttachedDiskSpec]=None,
        network_interface_specs: List[network_models.NetworkInterfaceSchema]=None, hostname=None,
        fqdn=None, underlay_networks: List[network_models.UnderlayNetworkSchema]=None,
        metadata=None, placement_policy: instance_models.PlacementPolicy=None,
        cauth=None, scheduling_policy: instance_models.SchedulingPolicy=None, internal_data_disk_id=None,
        service_account_id=None, hypervisor_type=None, nested_virtualization=None, network_settings=None,
        disable_seccomp=None, dry_run=False, pci_topology_id=None,
        **kwargs
    ) -> instance_models.InstanceOperation:
        return self.__post_operation("/instances", self.__get_create_instance_request(
            folder_id=folder_id,
            zone_id=zone_id,

            name=name,
            labels=labels,
            description=description,

            platform_id=platform_id,
            resources=resources,
            hypervisor_type=hypervisor_type,
            nested_virtualization=nested_virtualization,
            placement_policy=placement_policy,
            scheduling_policy=scheduling_policy,
            pci_topology_id=pci_topology_id,

            boot_disk_spec=boot_disk_spec,
            secondary_disk_specs=secondary_disk_specs,

            network_interface_specs=network_interface_specs,
            hostname=hostname,
            fqdn=fqdn,
            underlay_networks=underlay_networks,
            network_settings=network_settings,

            metadata=metadata,
            cauth=cauth,
            internal_data_disk_id=internal_data_disk_id,
            service_account_id=service_account_id,
            disable_seccomp=disable_seccomp,
            dry_run=dry_run,
        ), model=instance_models.InstanceOperation, **kwargs)

    def simulate_instance_billing_metrics(
        self, *,
        folder_id, name, zone_id, platform_id, resources: instance_models.InstanceResourcesSpec,
        description=None, labels=None, boot_disk_spec: nbs_models.AttachedDiskSpec=None,
        secondary_disk_specs: List[nbs_models.AttachedDiskSpec]=None,
        network_interface_specs: List[network_models.NetworkInterfaceSchema]=None, hostname=None,
        fqdn=None, underlay_networks: List[network_models.UnderlayNetworkSchema]=None,
        metadata=None, placement_policy: instance_models.PlacementPolicy=None,
        cauth=None, scheduling_policy: instance_models.SchedulingPolicy=None, internal_data_disk_id=None,
        hypervisor_type=None, nested_virtualization=None, network_settings=None, disable_seccomp=None
    ) -> console_models.BillingMetricsListModel:
        return self.__client.post(
            self.__api_path("/console/simulateInstanceBillingMetrics"),
            self.__get_create_instance_request(
                folder_id=folder_id,
                zone_id=zone_id,

                name=name,
                description=description,
                labels=labels,

                platform_id=platform_id,
                resources=resources,
                hypervisor_type=hypervisor_type,
                nested_virtualization=nested_virtualization,
                placement_policy=placement_policy,
                scheduling_policy=scheduling_policy,

                boot_disk_spec=boot_disk_spec,
                secondary_disk_specs=secondary_disk_specs,

                network_interface_specs=network_interface_specs,
                hostname=hostname,
                fqdn=fqdn,
                underlay_networks=underlay_networks,
                network_settings=network_settings,

                metadata=metadata,
                cauth=cauth,
                internal_data_disk_id=internal_data_disk_id,
                disable_seccomp=disable_seccomp,
            ), model=console_models.BillingMetricsListModel,
        ).metrics

    @staticmethod
    def __get_create_instance_request(
        *, folder_id, name, zone_id, platform_id, resources: instance_models.InstanceResourcesSpec,
        description=None, labels=None, boot_disk_spec: nbs_models.AttachedDiskSpec=None,
        secondary_disk_specs: List[nbs_models.AttachedDiskSpec]=None,
        network_interface_specs: List[network_models.NetworkInterfaceSchema]=None, hostname=None,
        fqdn=None, underlay_networks: List[network_models.UnderlayNetworkSchema]=None,
        metadata=None, placement_policy: instance_models.PlacementPolicy=None,
        cauth=None, scheduling_policy: instance_models.SchedulingPolicy=None, internal_data_disk_id=None,
        service_account_id=None, hypervisor_type=None, nested_virtualization=None, network_settings=None,
        disable_seccomp=None, dry_run=False, pci_topology_id=None
    ):
        return drop_none({
            "folderId": folder_id,
            "zoneId": zone_id,

            "name": name,
            "description": description,
            "labels": labels,

            "platformId": platform_id,
            "resourcesSpec": resources,
            "hypervisorType": hypervisor_type,
            "nestedVirtualization": nested_virtualization,
            "placementPolicy": placement_policy,
            "schedulingPolicy": scheduling_policy,
            "pciTopologyId": pci_topology_id,

            "bootDiskSpec": boot_disk_spec,
            "secondaryDiskSpecs": secondary_disk_specs,

            "networkInterfaceSpecs": network_interface_specs,
            "hostname": hostname,
            "fqdn": fqdn,
            "underlayNetworks": underlay_networks,
            "networkSettings": network_settings,

            "metadata": metadata,
            "cauth": cauth,
            "internalDataDiskId": internal_data_disk_id,
            "serviceAccountId": service_account_id,
            "disableSeccomp": disable_seccomp,

            "dryRun": dry_run,
        })

    def update_instance(
        self, instance_id, name=None, description=None, labels=None, metadata=None, service_account_id=None,
        placement=None, placement_policy: instance_models.PlacementPolicyUpdate=None,
        scheduling_policy: instance_models.SchedulingPolicyUpdate=None, boot_disk_spec=None,
        keep_auto_deleted_disk=False, platform_id=None, resources_spec: instance_models.InstanceResourcesSpec=None,
        network_settings: network_models.NetworkSettingsUpdate=None, update_mask: List[str]=None, **kwargs
    ) -> instance_models.InstanceOperation:
        return self.__patch_operation(
            "/instances/" + instance_id,
            drop_none({
                "name": name,
                "description": description,
                "labels": labels,
                "metadata": metadata,
                "serviceAccountId": service_account_id,
                "placement": placement,
                "placementPolicy": placement_policy,
                "schedulingPolicy": scheduling_policy,
                "bootDiskSpec": boot_disk_spec,
                "keepAutoDeletedDisk": keep_auto_deleted_disk,
                "platformId": platform_id,
                "resourcesSpec": resources_spec,
                "networkSettings": network_settings,
                "updateMask": ",".join(update_mask) if update_mask is not None else None,
            }),
            model=instance_models.InstanceOperation,
            **kwargs
        )

    def update_instance_metadata(self, instance_id, upsert=None, delete=None, **kwargs) -> instance_models.InstanceOperation:
        return self.__post_operation(
            "/instances/" + instance_id + "/updateMetadata",
            drop_none({
                "upsert": upsert,
                "delete": delete,
            }),
            model=instance_models.InstanceOperation,
            **kwargs
        )

    def start_instance(self, instance_id, **kwargs) -> instance_models.InstanceOperation:
        return self.__post_operation(
            "/instances/" + instance_id + "/start",
            {}, model=instance_models.InstanceOperation,
            **kwargs
        )

    def stop_instance(self, instance_id: str, force: bool=False, compute_node: str=None,
                      termination_grace_period: int=None, **kwargs) -> instance_models.InstanceOperation:
        return self.__post_operation(
            "/instances/" + instance_id + "/stop",
            drop_none({
                "force": force,
                "compute_node": compute_node,
                "termination_grace_period": termination_grace_period,
            }),
            model=instance_models.InstanceOperation,
            **kwargs
        )

    def restart_instance(self, instance_id, force=False, **kwargs) -> instance_models.InstanceOperation:
        return self.__post_operation(
            "/instances/" + instance_id + "/restart",
            drop_none({
                "force": force,
            }),
            model=instance_models.InstanceOperation,
            **kwargs
        )

    def get_instance_serial_port_output(self, instance_id, port=None) -> instance_models.InstanceSerialPort:
        return self.__get_response(
            "/instances/" + instance_id + "/serialPortOutput",
            params=drop_none({"port": port}),
            model=instance_models.InstanceSerialPort)

    def delete_instance(self, instance_id, safe_delete=None, **kwargs) -> instance_models.InstanceOperation:
        return self.__delete_operation(
            "/instances/" + instance_id,
            model=instance_models.InstanceOperation,
            params=drop_none({
                "safe_delete": safe_delete,
            }),
            **kwargs
        )

    def force_deallocate(self, instance_id: str, ignore_suspicions: bool=None, **kwargs) -> instance_models.InstanceOperation:
        return self.__post_operation(
            "/instances/" + instance_id + "/forceDeallocate",
            drop_none({
                "ignoreSuspicions": ignore_suspicions,
            }),
            model=instance_models.InstanceOperation,
            **kwargs
        )

    def migrate_instance(self, source_compute_node, instance_id, *, target_compute_nodes=None, emulate_failure=None, **kwargs) -> instance_models.InstanceOperation:
        return self.__post_operation("/instances/" + instance_id + "/migrate", drop_none({
            "source_compute_node": source_compute_node,
            "target_compute_nodes": target_compute_nodes,
            "emulate_failure": emulate_failure,
        }), model=instance_models.InstanceOperation, **kwargs)

    def migrate_instance_offline(self, instance_id: str, compute_node: str, **kwargs) -> instance_models.InstanceOperation:
        return self.__post_operation("/instances/" + instance_id + "/migrateOffline", {
            "compute_node": compute_node,
        }, model=instance_models.InstanceOperation, **kwargs)

    def add_instance_one_to_one_nat(self, instance_id, network_interface_index, one_to_one_nat_spec, internal_address=None, **kwargs) -> instance_models.InstanceOperation:
        return self.__post_operation(
            "/instances/" + instance_id + "/addOneToOneNat",
            drop_none({
                "networkInterfaceIndex": network_interface_index,
                "internalAddress": internal_address,
                "oneToOneNatSpec": one_to_one_nat_spec,
            }),
            model=instance_models.InstanceOperation,
            **kwargs
        )

    def remove_instance_one_to_one_nat(self, instance_id, network_interface_index, internal_address=None, **kwargs) -> instance_models.InstanceOperation:
        return self.__post_operation(
            "/instances/" + instance_id + "/removeOneToOneNat",
            drop_none({
                "networkInterfaceIndex": network_interface_index,
                "internalAddress": internal_address,
            }),
            model=instance_models.InstanceOperation,
            **kwargs
        )

    # Disks

    def attach_disk(self, instance_id, disk_spec: nbs_models.AttachedDiskSpec = None, disk_id: str = None, **kwargs) -> instance_models.AttachInstanceDiskOperation:
        if not disk_id and not disk_spec:
            raise Exception("One of disk_id or disk_spec is required")

        return self.__post_operation(
            "/instances/" + instance_id + "/attachDisk",
            drop_none({"disk_spec": disk_spec if disk_spec else nbs_models.AttachedDiskSpec.new(disk_id=disk_id)}),
            model=instance_models.AttachInstanceDiskOperation,
            **kwargs
        )

    # only one parameter is allowed(and required), disk_id or device_name
    def detach_disk(self, instance_id, disk_id=None, device_name=None, **kwargs) -> instance_models.DetachInstanceDiskOperation:
        return self.__post_operation(
            "/instances/" + instance_id + "/detachDisk",
            drop_none({
                "disk_id": disk_id,
                "device_name": device_name
            }),
            model=instance_models.DetachInstanceDiskOperation,
            **kwargs
        )

    # Networks

    def iter_network_operations(self, network_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_network_operations, "operations", page_size, network_id)

    def list_network_operations(self, network_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/networks/" + network_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=operations_models.OperationListV1Beta1)

    def iter_networks(self, folder_id, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[network_models.Network]:
        return paging.iter_items(self.list_networks, "networks", page_size, folder_id, name=name)

    def list_networks(self, folder_id, name=None, page_size=None, page_token=None) -> network_models.NetworkList:
        return self.__get_response("/networks", params=drop_none({
            "folderId": folder_id,
            "name": name,
            "pageSize": page_size,
            "pageToken": page_token,
        }), model=network_models.NetworkList)

    def get_network(self, network_id) -> network_models.Network:
        return self.__get_response("/networks/" + network_id, model=network_models.Network)

    def create_network(self, folder_id, name=None, description=None, labels=None, **kwargs) -> network_models.NetworkOperation:
        return self.__post_operation(
            "/networks",
            drop_none({
                "folderId": folder_id,
                "name": name,
                "description": description,
                "labels": labels,
            }),
            model=network_models.NetworkOperation,
            **kwargs
        )

    def update_network(self, network_id, name=None, description=None, labels=None, **kwargs) -> network_models.NetworkOperation:
        return self.__patch_operation(
            "/networks/" + network_id,
            drop_none({
                "name": name,
                "description": description,
                "labels": labels,
            }),
            model=network_models.NetworkOperation,
            **kwargs
        )

    def delete_network(self, network_id, **kwargs) -> network_models.NetworkOperation:
        return self.__delete_operation(
            "/networks/" + network_id,
            model=network_models.NetworkOperation,
            **kwargs
        )

    def move_network(self, network_id: str, destination_folder_id: str, **kwargs) -> network_models.NetworkOperation:
        return self.__post_operation(
            "/networks/" + network_id + "/move",
            {
                "destinationFolderId": destination_folder_id,
            },
            model=network_models.NetworkOperation,
            **kwargs
        )

    # Network defaults

    def create_network_system_route_table(self, network_id: str, **kwargs) -> network_models.NetworkOperation:
        return self.__post_operation(
            "/networks/" + network_id + '/systemRouteTable',
            {},
            model=network_models.NetworkOperation,
            **kwargs
        )

    def create_default_network(self, folder_id, name=None, **kwargs) -> network_models.CreateDefaultNetworkOperation:
        return self.__post_operation(
            "/defaultNetwork",
            drop_none({
                "folderId": folder_id,
                "name": name,
            }),
            model=network_models.CreateDefaultNetworkOperation,
            **kwargs
        )

    # Network DNS API

    def list_network_dns_records(self, network_id, **kwargs) -> network_models.NetworkDnsRecordsList:
        return self.__get_response(
            "/networks/" + network_id + "/dnsRecords",
            model=network_models.NetworkDnsRecordsList,
            **kwargs
        )

    def set_network_dns_record(self, network_id, type, name, value, ttl=None, **kwargs) -> network_models.NetworkOperation:
        return self.__post_operation(
            "/networks/" + network_id + "/setDnsRecord",
            drop_none({
                "type": type,
                "name": name,
                "value": value,
                "ttl": ttl,
            }),
            model=network_models.NetworkOperation,
            **kwargs
        )

    def delete_network_dns_record(self, network_id, type, name, value=None, **kwargs) -> network_models.NetworkOperation:
        return self.__post_operation(
            "/networks/" + network_id + "/deleteDnsRecord",
            drop_none({
                "type": type,
                "name": name,
                "value": value,
            }),
            model=network_models.NetworkOperation,
            **kwargs
        )

    # Subnets

    def iter_subnet_operations(self, subnet_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_subnet_operations, "operations", page_size, subnet_id)

    def list_subnet_operations(self, subnet_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/subnets/" + subnet_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=operations_models.OperationListV1Beta1)

    def iter_network_subnets(self, network_id, zone_id=None, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[network_models.Subnet]:
        return paging.iter_items(self.list_network_subnets, "subnets", page_size, network_id, zone_id=zone_id, name=name)

    def list_network_subnets(self, network_id, zone_id=None, name=None, page_size=None, page_token=None) -> network_models.SubnetList:
        return self.__get_response("/networks/" + network_id + "/subnets", params=drop_none({
            "pageSize": page_size,
            "pageToken": page_token,
            "zoneId": zone_id,
            "name": name,
        }), model=network_models.SubnetList)

    def iter_subnets(self, folder_id, network_id=None, zone_id=None, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[network_models.Subnet]:
        return paging.iter_items(self.list_subnets, "subnets", page_size, folder_id, network_id=network_id, zone_id=zone_id, name=name)

    def list_subnets(self, folder_id, network_id=None, zone_id=None, name=None, page_size=None, page_token=None) -> network_models.SubnetList:
        return self.__get_response("/subnets", params=drop_none({
            "folderId": folder_id,
            "networkId": network_id,
            "zoneId": zone_id,
            "name": name,
            "pageSize": page_size,
            "pageToken": page_token,
        }), model=network_models.SubnetList)

    def get_subnet(self, subnet_id) -> network_models.Subnet:
        return self.__get_response("/subnets/" + subnet_id, model=network_models.Subnet)

    def create_subnet(self, folder_id, network_id, zone_id, name=None, v4_cidr_blocks=None, v6_cidr_blocks=None,
                      description=None, labels=None, route_table_id=None, extra_params=None, egress_nat_enable=None,
                      v4_cidr_block=None, v6_cidr_block=None,
                      **kwargs) -> network_models.SubnetOperation:
        # FIXME(CLOUD-30682): Legacy
        if v4_cidr_blocks is None:
            v4_cidr_blocks = v4_cidr_block
        if v6_cidr_blocks is None:
            v6_cidr_blocks = v6_cidr_block

        return self.__post_operation(
            "/subnets",
            drop_none({
                "folderId": folder_id,
                "networkId": network_id,
                "zoneId": zone_id,
                "name": name,
                # FIXME: rename to *Blocks
                "v4CidrBlock": v4_cidr_blocks,
                "v6CidrBlock": v6_cidr_blocks,
                "extraParams": extra_params,
                "description": description,
                "labels": labels,
                "routeTableId": route_table_id,
                "egressNatEnable": egress_nat_enable,
            }),
            model=network_models.SubnetOperation,
            **kwargs
        )

    def update_subnet(self, subnet_id, name=_UNSET, description=_UNSET, labels=_UNSET,
                      route_table_id=_UNSET, export_rts=_UNSET, import_rts=_UNSET, egress_nat_enable=_UNSET,
                      **kwargs) -> network_models.SubnetOperation:
        update = {
            "name": name,
            "description": description,
            "labels": labels,
            "route_table_id": route_table_id,
            "egress_nat_enable": egress_nat_enable,
        }
        update_mask = ",".join(field_name for field_name, value in update.items() if value is not _UNSET)

        return self.__patch_operation(
            "/subnets/" + subnet_id,
            drop_none({
                "name": name,
                "description": description,
                "labels": labels,
                "routeTableId": route_table_id,
                "egressNatEnable": egress_nat_enable,
                "updateMask": update_mask,
            }, none=_UNSET),
            model=network_models.SubnetOperation,
            **kwargs
        )

    def delete_subnet(self, subnet_id, **kwargs) -> network_models.SubnetOperation:
        return self.__delete_operation(
            "/subnets/" + subnet_id,
            model=network_models.SubnetOperation,
            **kwargs
        )

    def move_subnet(self, subnet_id: str, destination_folder_id: str, **kwargs) -> network_models.SubnetOperation:
        return self.__post_operation(
            "/subnets/" + subnet_id + "/move",
            {
                "destinationFolderId": destination_folder_id,
            },
            model=network_models.SubnetOperation,
            **kwargs
        )

    # Addresses

    def iter_address_operations(self, address_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_address_operations, "operations", page_size, address_id)

    def list_address_operations(self, address_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/addresses/" + address_id + "/operations", params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }), model=operations_models.OperationListV1Beta1)

    def iter_addresses(self, folder_id, type, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[network_models.Address]:
        return paging.iter_items(self.list_addresses, "addresses", page_size, folder_id, type, name=name)

    def list_addresses(self, folder_id, type, name=None, page_size=None, page_token=None) -> network_models.AddressList:
        return self.__get_response(
            "/addresses", params=drop_none({
                "folderId": folder_id,
                "type": type,
                "name": name,
                "pageSize": page_size,
                "pageToken": page_token,
            }), model=network_models.AddressList)

    def get_address(self, address_id) -> network_models.Address:
        return self.__get_response("/addresses/" + address_id, model=network_models.Address)

    def create_address(self, folder_id, name=None, description=None, labels=None, external_address_spec=None, internal_address_spec=None, ephemeral=None, **kwargs) -> network_models.AddressOperation:
        return self.__post_operation(
            "/addresses",
            drop_none({
                "folderId": folder_id,
                "name": name,
                "description": description,
                "labels": labels,
                "externalAddressSpec": external_address_spec,
                "internalAddressSpec": internal_address_spec,
                "ephemeral": ephemeral,
            }),
            model=network_models.AddressOperation,
            **kwargs
        )

    def update_address(self, address_id, name=None, description=None, labels=None, ephemeral=None, **kwargs) -> network_models.AddressOperation:
        return self.__patch_operation(
            "/addresses/" + address_id,
            drop_none({
                "name": name,
                "description": description,
                "labels": labels,
                "ephemeral": ephemeral,
            }),
            model=network_models.AddressOperation,
            **kwargs
        )

    def delete_address(self, address_id, **kwargs) -> network_models.AddressOperation:
        return self.__delete_operation(
            "/addresses/" + address_id,
            model=network_models.AddressOperation,
            **kwargs
        )

    # Fip buckets

    def create_fip_bucket(self, folder_id, flavor, scope, cidrs,
                          import_rts=None, export_rts=None, ip_version=None, **kwargs) -> network_models.FipBucketOperation:
        return self.__post_operation(
            "/fipBuckets",
            drop_none({
                "folderId": folder_id,
                "flavor": flavor,
                "scope": scope,
                "cidrs": cidrs,
                "importRts": import_rts,
                "exportRts": export_rts,
                "ipVersion": ip_version,
            }),
            model=network_models.FipBucketOperation,
            **kwargs
        )

    def delete_fip_bucket(self, bucket_id, **kwargs) -> network_models.FipBucketOperation:
        return self.__delete_operation(
            "/fipBuckets/" + bucket_id,
            model=network_models.FipBucketOperation,
            **kwargs
        )

    def list_fip_buckets(self, folder_id) -> network_models.FipBucketList:
        return self.__get_response(
            "/fipBuckets",
            params=drop_none({
                "folderId": folder_id
            }),
            model=network_models.FipBucketList
        )

    def get_fip_bucket(self, bucket_id) -> network_models.FipBucket:
        return self.__get_response(
            "/fipBuckets/" + bucket_id,
            model=network_models.FipBucket
        )

    def add_fip_bucket_cidrs(self, bucket_id, cidrs, **kwargs) -> network_models.FipBucketOperation:
        return self.__post_operation(
            "/fipBuckets/" + bucket_id + "/add-cidrs",
            drop_none({
                "cidrs": cidrs,
            }),
            model=network_models.FipBucketOperation,
            **kwargs
        )

    def delete_fip_bucket_cidrs(self, bucket_id, cidrs, **kwargs) -> network_models.FipBucketOperation:
        return self.__post_operation(
            "/fipBuckets/" + bucket_id + "/delete-cidrs",
            drop_none({
                "cidrs": cidrs,
            }),
            model=network_models.FipBucketOperation,
            **kwargs
        )

    # Route tables

    def iter_route_tables_operations(self, route_table_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_route_tables_operations, "operations", page_size, route_table_id)

    def list_route_tables_operations(self, route_table_id: str, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/routeTables/" + route_table_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=operations_models.OperationListV1Beta1
        )

    def iter_network_route_tables(self, network_id, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[network_models.RouteTable]:
        return paging.iter_items(self.list_network_route_tables, "route_tables", page_size, network_id, name=name)

    def list_network_route_tables(self, network_id, name=None, page_size=None, page_token=None) -> network_models.RouteTableList:
        return self.__get_response("/networks/" + network_id + "/routeTables", params=drop_none({
            "pageSize": page_size,
            "pageToken": page_token,
            "name": name,
        }), model=network_models.RouteTableList)

    def iter_route_tables(self, folder_id, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[network_models.RouteTable]:
        return paging.iter_items(self.list_route_tables, "route_tables", page_size, folder_id, name=name)

    def list_route_tables(self, folder_id, name=None, page_size=None, page_token=None) -> network_models.RouteTableList:
        return self.__get_response(
            "/routeTables",
            params=drop_none({
                "folderId": folder_id,
                "name": name,
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=network_models.RouteTableList
        )

    def get_route_table(self, route_table_id: str) -> network_models.RouteTable:
        return self.__get_response(
            "/routeTables/" + route_table_id,
            model=network_models.RouteTable
        )

    def create_route_table(self,
                           folder_id,
                           network_id,
                           static_routes,
                           name=None,
                           description=None,
                           labels=None,
                           **kwargs) -> network_models.RouteTableOperation:
        return self.__post_operation(
            "/routeTables",
            drop_none({
                "folderId": folder_id,
                "name": name,
                "description": description,
                "labels": labels,
                "network_id": network_id,
                "static_routes": static_routes,
            }),
            model=network_models.RouteTableOperation,
            **kwargs
        )

    def update_route_table(self,
                           route_table_id: str,
                           static_routes=_UNSET,
                           name=_UNSET,
                           description=_UNSET,
                           labels=_UNSET,
                           **kwargs) -> network_models.RouteTableOperation:
        update = {
            "name": name,
            "description": description,
            "labels": labels,
            "static_routes": static_routes,
        }
        update['updateMask'] = ",".join(field_name for field_name, value in update.items() if value is not _UNSET)

        return self.__patch_operation(
            "/routeTables/" + route_table_id,
            drop_none(update, none=_UNSET),
            model=network_models.RouteTableOperation,
            **kwargs
        )

    def update_route_table_static_routes(self,
                                         route_table_id: str,
                                         delete: list = None,
                                         upsert: list = None,
                                         **kwargs) -> network_models.RouteTableOperation:
        return self.__post_operation(
            "/routeTables/" + route_table_id + "/updateStaticRoutes",
            drop_none({
                "upsert": upsert,
                "delete": delete,
            }),
            model=network_models.RouteTableOperation,
            **kwargs
        )

    def delete_route_table(self, route_table_id: str, **kwargs) -> network_models.RouteTableOperation:
        return self.__delete_operation(
            "/routeTables/" + route_table_id,
            model=network_models.RouteTableOperation,
            **kwargs
        )

    def move_route_table(self, route_table_id: str, destination_folder_id: str, **kwargs) -> network_models.RouteTableOperation:
        return self.__post_operation(
            "/routeTables/" + route_table_id + "/move",
            {
                "destinationFolderId": destination_folder_id,
            },
            model=network_models.RouteTableOperation,
            **kwargs
        )

    # Disk types

    def iter_disk_types(self, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[nbs_models.DiskType]:
        return paging.iter_items(self.list_disk_types, "disk_types", page_size)

    def list_disk_types(self, page_size=None, page_token=None) -> nbs_models.DiskTypeList:
        return self.__get_response(
            "/diskTypes",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=nbs_models.DiskTypeList
        )

    def get_disk_type(self, disk_type_id: nbs_models.DiskTypeId) -> nbs_models.DiskType:
        disk_type_id = str(disk_type_id) if disk_type_id is not None else None  # Convert DiskTypeId enum into string
        return self.__get_response(
            "/diskTypes/" + disk_type_id,
            model=nbs_models.DiskType
        )

    # Disks

    def create_disk(self, *, folder_id, size, zone_id, type_id=None,
                    image_id=None, snapshot_id=None,
                    name=None, description=None, labels=None, **kwargs) -> nbs_models.DiskOperation:
        type_id = str(type_id) if type_id is not None else None  # Convert DiskTypeId enum into string
        return self.__post_operation(
            "/disks",
            drop_none({
                "folderId": folder_id,
                "name": name,
                "size": size,
                "zoneId": zone_id,
                "typeId": type_id,
                "imageId": image_id,
                "snapshotId": snapshot_id,
                "description": description,
                "labels": labels,
            }),
            model=nbs_models.DiskOperation,
            **kwargs
        )

    def delete_disk(self, disk_id, safe_delete=None, **kwargs) -> nbs_models.DiskOperation:
        url = "/disks/" + disk_id
        return self.__delete_operation(
            url,
            model=nbs_models.DiskOperation,
            params=drop_none({
                "safe_delete": safe_delete,
            }),
            **kwargs
        )

    def restore_removed_disk(self, disk_id, **kwargs):
        url = "/admin/disks/" + disk_id + "/restore"
        return self.__post_response(
            url, 
            {}, 
            **kwargs
        )

    def update_disk(self, disk_id, name=None, description=None, labels=None, size=None, **kwargs) -> nbs_models.DiskOperation:
        update_mask = {key for key, value in {"name": name, "description": description, "labels": labels, "size": size}.items() if value is not None}
        return self.__patch_operation(
            "/disks/" + disk_id,
            drop_none({
                "name": name,
                "updateMask": ",".join(update_mask) if update_mask is not None else None,
                "description": description,
                "labels": labels,
                "size": size,
            }),
            model=nbs_models.DiskOperation,
            **kwargs
        )

    def get_disk(self, disk_id) -> nbs_models.DiskV1Beta1:
        return self.__get_response(
            "/disks/" + disk_id,
            model=nbs_models.DiskV1Beta1,
        )

    def list_disks(self, folder_id, name=None, page_size=None, page_token=None) -> nbs_models.DiskListV1Beta1:
        return self.__get_response(
            "/disks",
            params=drop_none({
                "folderId": folder_id,
                "name": name,
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=nbs_models.DiskListV1Beta1
        )

    def iter_disks(self, project_id, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[nbs_models.DiskV1Beta1]:
        return paging.iter_items(self.list_disks, "disks", page_size, project_id, name=name)

    def iter_disk_operations(self, disk_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_disk_operations, "operations", page_size, disk_id)

    def list_disk_operations(self, disk_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/disks/" + disk_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=operations_models.OperationListV1Beta1)

    # Snapshots

    def iter_snapshot_operations(self, snapshot_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_snapshot_operations, "operations", page_size, snapshot_id)

    def list_snapshot_operations(self, snapshot_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/snapshots/" + snapshot_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=operations_models.OperationListV1Beta1)

    def get_snapshot(self, snapshot_id) -> snapshot_models.SnapshotV1Beta1:
        return self.__get_response("/snapshots/" + snapshot_id, model=snapshot_models.SnapshotV1Beta1)

    def iter_snapshots(self, folder_id, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[snapshot_models.SnapshotV1Beta1]:
        return paging.iter_items(self.list_snapshots, "snapshots", page_size, folder_id, name=name)

    def list_snapshots(self, folder_id, name=None, page_size=None, page_token=None) -> snapshot_models.SnapshotListV1Beta1:
        return self.__get_response("/snapshots", params=drop_none({
            "folderId": folder_id,
            "name": name,
            "pageSize": page_size,
            "pageToken": page_token,
        }), model=snapshot_models.SnapshotListV1Beta1)

    def create_snapshot(self, folder_id, disk_id, name=None, description=None, labels=None, **kwargs) -> snapshot_models.CreateSnapshotOperation:
        return self.__post_operation(
            "/snapshots",
            drop_none({
                "folderId": folder_id,
                "diskId": disk_id,
                "name": name,
                "description": description,
                "labels": labels,
            }),
            model=snapshot_models.CreateSnapshotOperation,
            **kwargs
        )

    def update_snapshot(self, snapshot_id, name=None, description=None, labels=None, update_mask=None, **kwargs) -> snapshot_models.SnapshotOperation:
        if update_mask is None:
            update_mask = {key for key, value in {"name": name, "description": description, "labels": labels}.items() if value is not None}
        return self.__patch_operation(
            "/snapshots/" + snapshot_id,
            drop_none({
                "updateMask": ",".join(update_mask) if update_mask is not None else None,
                "name": name,
                "description": description,
                "labels": labels,
            }),
            model=snapshot_models.SnapshotOperation,
            **kwargs
        )

    def delete_snapshot(self, snapshot_id, **kwargs) -> snapshot_models.SnapshotOperation:
        return self.__delete_operation(
            "/snapshots/" + snapshot_id,
            model=snapshot_models.SnapshotOperation,
            **kwargs
        )

    # Availability zones

    def iter_zones(self, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[zones_models.Zone]:
        return paging.iter_items(self.list_zones, "zones", page_size)

    def list_zones(self, page_size=None, page_token=None) -> zones_models.ZonesList:
        return self.__get_response(
            "/zones",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=zones_models.ZonesList
        )

    def get_zone(self, zone_id) -> zones_models.Zone:
        return self.__get_response(
            "/zones/" + zone_id,
            model=zones_models.Zone,
        )

    def create_zone(self, zone_id, **kwargs) -> zones_models.ZoneOperation:
        return self.__post_operation(
            "/zones",
            drop_none({"zoneId": zone_id}),
            model=zones_models.ZoneOperation,
            **kwargs
        )

    def delete_zone(self, zone_id, **kwargs) -> zones_models.ZoneOperation:
        return self.__delete_operation(
            "/zones/" + zone_id,
            model=zones_models.ZoneOperation,
            **kwargs
        )

    def update_zone(self, zone_id, status=None, priority=None, **kwargs) -> zones_models.ZoneOperation:
        return self.__patch_operation(
            "/zones/" + zone_id,
            drop_none({
                "status": status,
                "priority": priority,
            }),
            model=zones_models.ZoneOperation,
            **kwargs
        )

    # Availability zones (special methods for identity)

    def update_zone_weights_for_identity(self, weights, **kwargs) -> zones_models.ZoneOperation:
        return self.__post_operation(
            "/identity/zones/updateWeights",
            drop_none({
                "weights": weights,
            }),
            model=zones_models.ZoneOperation,
            **kwargs
        )

    def iter_zones_for_identity(self, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[zones_models.Zone]:
        return paging.iter_items(self.list_zones_for_identity, "zones", page_size)

    def list_zones_for_identity(self, page_size=None, page_token=None) -> zones_models.ZonesList:
        return self.__get_response(
            "/identity/zones",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=zones_models.ZonesList
        )

    # Special API for identity
    def identity_authorize(self, instance_id, service_account_id, node_fqdn):
        return self.__get_response("/identity/authorize", params={
            "node_fqdn": node_fqdn,
            "instance_id": instance_id,
            "service_account_id": service_account_id,
        })

    # Images

    def iter_image_operations(self, image_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_image_operations, "operations", page_size, image_id)

    def list_image_operations(self, image_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/images/" + image_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=operations_models.OperationListV1Beta1)

    def iter_images(self, folder_id, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[images_models.Image]:
        return paging.iter_items(self.list_images, "images", page_size, folder_id, name=name)

    def list_images(self, folder_id, name=None, page_size=None, page_token=None) -> images_models.ImageList:
        return self.__get_response("/images", params=drop_none({
            "folderId": folder_id,
            "pageSize": page_size,
            "pageToken": page_token,
            "name": name,
        }), model=images_models.ImageList)

    def get_image(self, image_id) -> images_models.Image:
        return self.__get_response("/images/" + image_id, model=images_models.Image)

    def get_image_latest_by_family(self, folder_id, family) -> images_models.Image:
        return self.__get_response(
            "/images:latestByFamily",
            params=drop_none({
                "folderId": folder_id,
                "family": family,
            }), model=images_models.Image)

    def create_image(self, folder_id,
                     name=None, disk_id=None, snapshot_id=None, uri=None, image_id=None,
                     description=None, family=None, labels=None,
                     product_ids=None, override_product_ids=None, os_type=None, min_disk_size=None,
                     requirements=None, ignore_whitelist=None,
                     **kwargs) -> images_models.ImageOperation:
        return self.__post_operation(
            "/images",
            drop_none({
                "folderId": folder_id,
                "name": name,
                "description": description,

                "diskId": disk_id,
                "snapshotId": snapshot_id,
                "uri": uri,
                "imageId": image_id,

                "minDiskSize": min_disk_size,
                "family": family,
                "labels": labels,
                "productIds": product_ids,
                "overrideProductIds": override_product_ids,
                "os": {"type": os_type.upper()} if os_type is not None else None,
                "requirements": requirements,
                "ignoreWhitelist": ignore_whitelist,
            }),
            model=images_models.ImageOperation,
            **kwargs
        )

    def update_image(self, image_id,
                     update_mask=None,
                     name=None, description=None, min_disk_size=None, labels=None, **kwargs) -> images_models.ImageOperation:
        return self.__patch_operation(
            "/images/" + image_id,
            drop_none({
                "updateMask": ",".join(update_mask) if update_mask is not None else None,
                "name": name,
                "description": description,
                "minDiskSize": min_disk_size,
                "labels": labels,
            }),
            model=images_models.ImageOperation,
            **kwargs
        )

    def delete_image(self, image_id, **kwargs) -> images_models.DeleteImageOperation:
        return self.__delete_operation(
            "/images/" + image_id,
            model=images_models.DeleteImageOperation,
            **kwargs
        )

    # Target groups

    def create_target_group(self, folder_id,
                            targets: List[target_groups_models.Target]=None,
                            name=None, description=None, labels=None,
                            **kwargs) -> target_groups_models.TargetGroupOperation:
        return self.__post_operation(
            "/targetGroups",
            drop_none({
                "folderId": folder_id,
                "name": name,
                "description": description,
                "labels": labels,

                "targets": targets,
            }),
            model=target_groups_models.TargetGroupOperation,
            **kwargs
        )

    def update_target_group(self, target_group_id,
                            targets: List[target_groups_models.Target]=None,
                            name=None, description=None, labels=None, **kwargs) -> target_groups_models.TargetGroupOperation:
        return self.__patch_operation(
            "/targetGroups/" + target_group_id,
            drop_none({
                "name": name,
                "description": description,
                "labels": labels,

                "targets": targets,
            }),
            model=target_groups_models.TargetGroupOperation,
            **kwargs
        )

    def delete_target_group(self, target_group_id, **kwargs) -> target_groups_models.TargetGroupOperation:
        return self.__delete_operation(
            "/targetGroups/" + target_group_id,
            model=target_groups_models.TargetGroupOperation,
            **kwargs
        )

    def get_target_group(self, target_group_id, **kwargs) -> target_groups_models.TargetGroup:
        return self.__get_response(
            "/targetGroups/" + target_group_id,
            model=target_groups_models.TargetGroup,
            **kwargs
        )

    def batch_get_target_groups(self,
                                target_group_ids,
                                folder_id=None,
                                **kwargs) -> target_groups_models.TargetGroupList:
        return self.__post_response(
            "/targetGroups:batchGet",
            request=drop_none({
                "folderId": folder_id,
                "targetGroupIds": target_group_ids,
            }),
            model=target_groups_models.TargetGroupList,
            **kwargs
        )

    def iter_target_groups(self, folder_id, name=None,
                           page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[target_groups_models.TargetGroup]:
        return paging.iter_items(self.list_target_groups, "target_groups", page_size, folder_id, name)

    def list_target_groups(self, folder_id,
                           name=None, page_size=None, page_token=None, **kwargs) -> target_groups_models.TargetGroupList:
        return self.__get_response(
            "/targetGroups",
            params=drop_none({
                "folderId": folder_id,
                "pageSize": page_size,
                "pageToken": page_token,
                "name": name,
            }),
            model=target_groups_models.TargetGroupList,
            **kwargs
        )

    def add_target_group_targets(self, target_group_id,
                                 targets: List[target_groups_models.Target],
                                 **kwargs) -> target_groups_models.TargetGroupOperation:
        return self.__post_operation(
            "/targetGroups/" + target_group_id + "/addTargets",
            drop_none({
                "targets": targets,
            }),
            model=target_groups_models.TargetGroupOperation,
            **kwargs
        )

    def remove_target_group_targets(self, target_group_id,
                                    targets: List[target_groups_models.Target],
                                    **kwargs) -> target_groups_models.TargetGroupOperation:
        return self.__post_operation(
            "/targetGroups/" + target_group_id + "/removeTargets",
            drop_none({
                "targets": targets,
            }),
            model=target_groups_models.TargetGroupOperation,
            **kwargs
        )

    def list_target_group_operations(self, target_group_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/targetGroups/" + target_group_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=operations_models.OperationListV1Beta1)

    # Network Load Balancer

    def create_network_load_balancer(self, folder_id, load_balancer_type,
                                     listener_specs: List[network_load_balancers_models.ListenerSpec]=None,
                                     attached_target_groups: List[target_groups_models.AttachedTargetGroupPublic]=None,
                                     name=None, description=None, labels=None, region_id=None,
                                     **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__post_operation(
            "/networkLoadBalancers",
            drop_none({
                "folderId": folder_id,
                "name": name,
                "description": description,
                "labels": labels,

                "region_id": region_id,
                "type": load_balancer_type,
                "listenerSpecs": listener_specs,
                "attachedTargetGroups": attached_target_groups,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def console_create_network_load_balancer(self, folder_id, load_balancer_type,
                                     listener_specs: List[network_load_balancers_models.ListenerSpec]=None,
                                     attached_target_group_specs: List[target_groups_models.AttachedTargetGroupPublic]=None,
                                     name=None, description=None, labels=None, region_id=None,
                                     **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__post_operation(
            "/console/networkLoadBalancers",
            drop_none({
                "folderId": folder_id,
                "name": name,
                "description": description,
                "labels": labels,

                "region_id": region_id,
                "type": load_balancer_type,
                "listenerSpecs": listener_specs,
                "attachedTargetGroupSpecs": attached_target_group_specs,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def update_network_load_balancer(self, network_load_balancer_id,
                                     listener_specs: List[network_load_balancers_models.ListenerSpec]=None,
                                     attached_target_groups: List[target_groups_models.AttachedTargetGroupPublic]=None,
                                     name=None, description=None, labels=None,
                                     **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__patch_operation(
            "/networkLoadBalancers/" + network_load_balancer_id,
            drop_none({
                "name": name,
                "description": description,
                "labels": labels,

                "listenerSpecs": listener_specs,
                "attachedTargetGroups": attached_target_groups,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def console_update_network_load_balancer(self, network_load_balancer_id,
                                     listener_specs: List[network_load_balancers_models.ListenerSpec]=None,
                                     attached_target_group_specs: List[target_groups_models.AttachedTargetGroupPublic]=None,
                                     name=None, description=None, labels=None,
                                     **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__patch_operation(
            "/console/networkLoadBalancers/" + network_load_balancer_id,
            drop_none({
                "name": name,
                "description": description,
                "labels": labels,

                "listenerSpecs": listener_specs,
                "attachedTargetGroupSpecs": attached_target_group_specs,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def get_network_load_balancer(self,
                                  network_load_balancer_id,
                                  model=network_load_balancers_models.NetworkLoadBalancerPublic,
                                  **kwargs) -> network_load_balancers_models.NetworkLoadBalancerPublic:
        return self.__get_response(
            "/networkLoadBalancers/" + network_load_balancer_id,
            model=model,
            **kwargs
        )

    def batch_get_network_load_balancers(self,
                                         network_load_balancer_ids,
                                         folder_id=None,
                                         **kwargs) -> network_load_balancers_models.NetworkLoadBalancerList:
        return self.__post_response(
            "/networkLoadBalancers:batchGet",
            request=drop_none({
                "folderId": folder_id,
                "networkLoadBalancerIds": network_load_balancer_ids,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerList,
            **kwargs
        )

    def iter_network_load_balancers(self, folder_id,
                                    name=None,
                                    page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[network_load_balancers_models.NetworkLoadBalancerPublic]:
        return paging.iter_items(self.list_network_load_balancers, "network_load_balancers", page_size, folder_id, name)

    def list_network_load_balancers(self, folder_id,
                                    name=None, page_size=None, page_token=None,
                                    **kwargs) -> network_load_balancers_models.NetworkLoadBalancerList:
        return self.__get_response(
            "/networkLoadBalancers",
            params=drop_none({
                "folderId": folder_id,
                "pageSize": page_size,
                "pageToken": page_token,
                "name": name,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerList,
            **kwargs
        )

    def get_target_states(self, network_load_balancer_id, target_group_id,
                          **kwargs) -> network_load_balancers_models.NetworkLoadBalancerTargetStates:
        return self.__get_response(
            "/networkLoadBalancers/" + network_load_balancer_id + "/getTargetStates",
            params=drop_none({
                "targetGroupId": target_group_id,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerTargetStates, api_version=self.NEXT_VERSION,
            **kwargs
        )

    def delete_network_load_balancer(self, network_load_balancer_id, **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__delete_operation(
            "/networkLoadBalancers/" + network_load_balancer_id,
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def start_network_load_balancer(self, network_load_balancer_id, **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__post_operation(
            "/networkLoadBalancers/" + network_load_balancer_id + "/start",
            {},
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def stop_network_load_balancer(self, network_load_balancer_id, **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__post_operation(
            "/networkLoadBalancers/" + network_load_balancer_id + "/stop",
            {},
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def attach_target_group(self, network_load_balancer_id,
                            target_group: target_groups_models.AttachedTargetGroupPublic,
                            **kwargs) -> network_load_balancers_models.NetworkLoadBalancerAttachmentOperation:
        return self.__post_operation(
            "/networkLoadBalancers/" + network_load_balancer_id + "/attachTargetGroup",
            drop_none({
                "attachedTargetGroup": target_group,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerAttachmentOperation,
            **kwargs
        )

    def detach_target_group(self, network_load_balancer_id, target_group_id,
                            **kwargs) -> network_load_balancers_models.NetworkLoadBalancerAttachmentOperation:
        return self.__post_operation(
            "/networkLoadBalancers/" + network_load_balancer_id + "/detachTargetGroup",
            drop_none({
                "targetGroupId": target_group_id,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerAttachmentOperation,
            **kwargs
        )

    def add_network_load_balancer_listener(self, network_load_balancer_id,
                                           listener_spec: network_load_balancers_models.ListenerSpecWithoutAntiDdos,
                                           **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__post_operation(
            "/networkLoadBalancers/" + network_load_balancer_id + "/addListener",
            drop_none({
                "listenerSpec": listener_spec,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def console_add_network_load_balancer_listener(self, network_load_balancer_id,
                                           listener_spec: network_load_balancers_models.ListenerSpec,
                                           **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__post_operation(
            "/console/networkLoadBalancers/" + network_load_balancer_id + "/addListener",
            drop_none({
                "listenerSpec": listener_spec,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def remove_network_load_balancer_listener(self, network_load_balancer_id, listener_name, **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__post_operation(
            "/networkLoadBalancers/" + network_load_balancer_id + "/removeListener",
            drop_none({
                "listener_name": listener_name,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def remove_network_load_balancer_listener_v1alpha1(self, network_load_balancer_id,
                                                       listener: network_load_balancers_models.ListenerPublicV1Alpha1,
                                                       **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        return self.__post_operation(
            "/networkLoadBalancers/" + network_load_balancer_id + "/removeListener",
            drop_none({
                "address": listener.address,
                "port": listener.port,
                "protocol": listener.protocol,
            }),
            model=network_load_balancers_models.NetworkLoadBalancerOperation,
            **kwargs
        )

    def list_network_load_balancer_operations(self, nlb_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/networkLoadBalancers/" + nlb_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=operations_models.OperationListV1Beta1,
        )

    # TODO(yesworld): Remove this after CLOUD-19587.
    def add_contrail_acl_rule(self, folder_id,
                              listeners: List[network_load_balancers_models.ListenerPublic],
                              targets: List[target_groups_models.Target], **kwargs) -> operations_models.OperationV1Beta1:
        return self.__post_operation("/networkLoadBalancers/addContrailAclRule", drop_none({
            "folderId": folder_id,
            "targets": targets,
            "listeners": listeners,
        }), model=operations_models.OperationV1Beta1, **kwargs)

    # TODO(yesworld): Remove this after CLOUD-19587.
    def remove_contrail_acl_rule(self, folder_id,
                                 listeners: List[network_load_balancers_models.ListenerPublic],
                                 targets: List[target_groups_models.Target], **kwargs) -> operations_models.OperationV1Beta1:
        return self.__post_operation("/networkLoadBalancers/removeContrailAclRule", drop_none({
            "folderId": folder_id,
            "targets": targets,
            "listeners": listeners,
        }), model=operations_models.OperationV1Beta1, **kwargs)

    # Console

    def get_vpc_folder_stats(self, folder_id) -> network_models.FolderStats:
        return self.__get_response("/console/vpc/folderStats/" + folder_id, model=network_models.FolderStats)

    def get_folder_stats(self, folder_id) -> console_models.FolderStats:
        return self.__get_response("/console/folderStats/" + folder_id, model=console_models.FolderStats)

    def iter_platforms(self) -> Iterable[console_models.Platform]:
        return iter(self.__get_response(
            "/console/platforms",
            model=console_models.PlatformsList,
        ).platforms)

    def iter_instance_disks(self, instance_id) -> Iterable[nbs_models.DiskV1Beta1]:
        return self.__get_response(
            "/console/instances/" + instance_id + "/disks",
            model=nbs_models.DiskListV1Beta1
        ).disks

    def get_instance_for_console(self, instance_id) -> instance_models.InstanceV1Beta1:
        return self.__get_response(
            "/console/instances/" + instance_id,
            model=instance_models.InstanceV1Beta1
        )

    def iter_instances_for_console(self, folder_id, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[instance_models.InstanceV1Beta1]:
        return paging.iter_items(self.list_instances_for_console, "instances", page_size, folder_id, name=name)

    def list_instances_for_console(self, folder_id, name=None, page_size=None, page_token=None) -> instance_models.InstanceListV1Beta1:
        """Returns portion of available instances"""

        return self.__get_response(
            "/console/instances",
            params=drop_none({
                "folderId": folder_id,
                "pageSize": page_size,
                "pageToken": page_token,
                "name": name,
            }),
            model=instance_models.InstanceListV1Beta1,
        )

    def iter_operations_for_console(self, folder_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_operations_for_console, "operations", page_size, folder_id)

    def list_operations_for_console(self, folder_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__get_response(
            "/console/operations", params=drop_none({
                "folderId": folder_id,
                "pageSize": page_size,
                "pageToken": page_token,
            }), model=operations_models.OperationListV1Beta1,
        )

    def iter_zones_for_console(self, cloud_id) -> Iterable[zones_models.Zone]:
        return iter(self.__get_response(
            "/console/zones",
            params={
                "cloudId": cloud_id,
            },
            model=zones_models.ZonesList,
        ).zones)

    # TODO(lavrukov): remove after transition period
    def console_get_vpc_objects(self, cloud_id: str, read_mask: List[str]=None) -> network_models.ConsoleVpcObjectsResponse:
        return self.__get_response(
            "/console/getVpcObjects",
            params=drop_none({
                "cloudId": cloud_id,
                "readMask": ",".join(read_mask) if read_mask is not None else None,
            }),
            model=network_models.ConsoleVpcObjectsResponse,
        )

    def get_response(self, url, model, params=None):
        return self.__get_response(url, params=drop_none(params or {}), model=model)

    # Quotas
    def get_cloud_quota(self, cloud_id: str) -> Quota:
        return self.__get_response("/quota/" + cloud_id, model=Quota)

    def update_cloud_limits(self, cloud_id: str, limits: Dict[str, int]) -> Quota:
        request = {
            "metrics": [{"name": name, "limit": limit} for name, limit in limits.items()],
        }
        path = "/quota/" + cloud_id
        return self.__client.patch(self.__api_path(path), request, model=Quota)

    def update_cloud_limit(self, cloud_id: str, name: str, limit: int) -> Quota:
        request = {
            "metric": {"name": name, "limit": limit},
        }
        path = "/quota/" + cloud_id + "/metric"
        return self.__client.patch(self.__api_path(path), request, model=Quota)

    def delete_cloud_limit(self, cloud_id: str, name: str) -> Quota:
        path = "/quota/" + cloud_id + "/metric/" + name
        return self.__client.delete(self.__api_path(path), model=Quota)

    def update_cloud_soft_limit(self, cloud_id: str, name: str, soft_limit: int) -> Quota:
        request = {
            "metric": {"name": name, "limit": soft_limit},
        }
        path = "/quota/" + cloud_id + "/softLimit"
        return self.__client.patch(self.__api_path(path), request, model=Quota)

    def delete_cloud_soft_limit(self, cloud_id: str, name: str) -> Quota:
        path = "/quota/" + cloud_id + "/softLimit/" + name
        return self.__client.delete(self.__api_path(path), model=Quota)

    def list_overridden_quota_metrics_by_name(self, folder_id: str, name: str, page_token=None, page_size=None) -> Quotas:
        path = "/quotaMetrics/" + name + "/listOverridden"
        return self.__client.get(self.__api_path(path), params=drop_none({
            "folderId": folder_id,
            "pageToken": page_token,
            "pageSize": page_size,
        }), model=Quotas)

    def iter_overridden_quota_metrics_by_name(self, folder_id: str, name: str, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[Quota]:
        return paging.iter_items(self.list_overridden_quota_metrics_by_name, "quotas", page_size, folder_id, name)

    def get_folder_quota(self, folder_id: str) -> FolderQuota:
        return self.__get_response("/folderQuota/" + folder_id, model=FolderQuota)

    def update_folder_limits(self, folder_id: str, limits: Dict[str, int]) -> FolderQuota:
        request = {
            "metrics": [{"name": name, "limit": limit} for name, limit in limits.items()],
        }
        path = "/folderQuota/" + folder_id
        return self.__client.patch(self.__api_path(path), request, model=FolderQuota)

    def update_folder_limit(self, folder_id: str, name: str, limit: int) -> FolderQuota:
        request = {
            "metric": {"name": name, "limit": limit},
        }
        path = "/folderQuota/" + folder_id + "/metric"
        return self.__client.patch(self.__api_path(path), request, model=FolderQuota)

    def delete_folder_limit(self, folder_id: str, name: str) -> FolderQuota:
        path = "/folderQuota/" + folder_id + "/metric/" + name
        return self.__client.delete(self.__api_path(path), model=FolderQuota)

    # Pooling

    def update_disk_pooling(self, image_id: str, zone_id=None, type_id=None, disk_count=1,
                            **kwargs) -> operations_models.OperationV1Beta1:
        return self.__patch_operation(
            "/pooling",
            drop_none({
                "imageId": image_id,
                "zoneId": zone_id,
                "typeId": type_id,
                "diskCount": disk_count,
            }),
            model=operations_models.OperationV1Beta1,
            **kwargs,
        )

    def get_disk_pooling(self, image_id=None, zone_id=None, type_id=None,
                         **kwargs) -> nbs_models.PoolingResponse:
        return self.__get_response(
            "/pooling",
            params=drop_none({
                "imageId": image_id,
                "zoneId": zone_id,
                "typeId": type_id,
            }),
            model=nbs_models.PoolingResponse,
            **kwargs,
        )

    def delete_disk_pooling(self, image_id: str, zone_id=None, type_id=None,
                            **kwargs) -> operations_models.OperationV1Beta1:
        return self.__delete_operation(
            "/pooling",
            params=drop_none({
                "imageId": image_id,
                "zoneId": zone_id,
                "typeId": type_id,
            }),
            model=operations_models.OperationV1Beta1,
            **kwargs,
        )

    # Solomon / resources

    def list_resource_types(self) -> List[str]:
        response = self.__get_response("/solomon/types", model=solomon_models.ResourceTypesList)
        return response.types

    def list_resources_by_type(self, folder_id, resource_type: str, page_token=None, page_size=None
                               ) -> solomon_models.ResourceList:
        return self.__get_response("/solomon/resources",
                                   params=drop_none({
                                       "folderId": folder_id,
                                       "pageToken": page_token,
                                       "pageSize": page_size,
                                       "resourceType": resource_type}),
                                   model=solomon_models.ResourceList)

    # Commitments

    def iter_commitment_operations(self, commitment_id, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[operations_models.OperationV1Beta1]:
        return paging.iter_items(self.list_commitment_operations, "operations", page_size, commitment_id)

    def list_commitment_operations(self, commitment_id, page_size=None, page_token=None) -> commitment_models.CommitmentOperationList:
        return self.__get_response(
            "/commitments/" + commitment_id + "/operations",
            params=drop_none({
                "pageSize": page_size,
                "pageToken": page_token,
            }),
            model=commitment_models.CommitmentOperationList)

    def iter_commitments(self, cloud_id, name=None, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[commitment_models.Commitment]:
        return paging.iter_items(self.list_commitments, "commitments", page_size, cloud_id, name=name)

    def list_commitments(self, cloud_id, name=None, page_size=None, page_token=None) -> commitment_models.CommitmentList:
        return self.__get_response("/commitments", params=drop_none({
            "cloudId": cloud_id,
            "name": name,
            "pageSize": page_size,
            "pageToken": page_token,
        }), model=commitment_models.CommitmentList)

    def get_commitment(self, commitment_id) -> commitment_models.Commitment:
        return self.__get_response("/commitments/" + commitment_id, model=commitment_models.Commitment)

    def create_commitment(self, *, cloud_id,
                          started_at, ended_at, platform_id, cores, memory,
                          name=None, description=None,
                          **kwargs) -> commitment_models.CommitmentOperation:
        return self.__post_operation(
            "/commitments",
            drop_none({
                "cloudId": cloud_id,
                "name": name,
                "description": description,

                "startedAt": started_at,
                "endedAt": ended_at,

                "platformId": platform_id,
                "cores": cores,
                "memory": memory,
            }),
            model=commitment_models.CommitmentOperation,
            **kwargs
        )

    def delete_commitment(self, commitment_id, force=False, **kwargs) -> commitment_models.CommitmentOperation:
        return self.__delete_operation(
            "/commitments/" + commitment_id,
            model=commitment_models.CommitmentOperation,
            params={"force": force},
            **kwargs
        )


class ComputeClientWithGrpcVpc(ComputeClient):
    def __init__(self, *args,
                 network_client,
                 subnet_client,
                 route_table_client,
                 address_client,
                 fip_bucket_client,
                 network_load_balancer_client,
                 target_group_client,
                 **kwargs):
        super().__init__(*args, **kwargs)
        self.__network_client = network_client
        self.__subnet_client = subnet_client
        self.__route_table_client = route_table_client
        self.__address_client = address_client
        self.__fip_bucket_client = fip_bucket_client
        self.__lb_client = network_load_balancer_client
        self.__tg_client = target_group_client

    # Networks

    def list_networks(self, folder_id, name=None, page_size=None, page_token=None) -> network_models.NetworkList:
        return self.__network_client.list(folder_id, filter=self._makefilter(name=name), page_size=page_size, page_token=page_token)

    def get_network(self, network_id) -> network_models.Network:
        return self.__network_client.get(network_id)

    def create_network(self, folder_id, name=None, description=None, labels=None, **kwargs) -> network_models.NetworkOperation:
        op = self.__network_client.create(folder_id, name=name, description=description, labels=labels)
        return self.__wait_operation(op, model=network_models.NetworkOperation, **kwargs)

    def update_network(self, network_id, name=None, description=None, labels=None, **kwargs) -> network_models.NetworkOperation:
        update = drop_none({
            "name": name,
            "description": description,
            "labels": labels,
        })
        update_mask = list(update)
        op = self.__network_client.update(network_id, update_mask=update_mask, **update)
        return self.__wait_operation(op, model=network_models.NetworkOperation, **kwargs)

    def delete_network(self, network_id, **kwargs) -> network_models.NetworkOperation:
        op = self.__network_client.delete(network_id)
        return self.__wait_operation(op, model=network_models.NetworkOperation, **kwargs)

    def move_network(self, network_id: str, destination_folder_id: str, **kwargs) -> network_models.NetworkOperation:
        op = self.__network_client.move(network_id, destination_folder_id)
        return self.__wait_operation(op, model=network_models.NetworkOperation, **kwargs)

    # Network defaults

    def create_network_system_route_table(self, network_id: str, **kwargs) -> network_models.NetworkOperation:
        op = self.__network_client.create_network_system_route_table(network_id)
        return self.__wait_operation(op, model=network_models.NetworkOperation, **kwargs)

    def create_default_network(self, folder_id, name=None, **kwargs) -> network_models.CreateDefaultNetworkOperation:
        op = self.__network_client.create_default_network(folder_id, name=name)
        return self.__wait_operation(op, model=network_models.CreateDefaultNetworkOperation, **kwargs)

    def list_network_subnets(self, network_id, zone_id=None, name=None, page_size=None, page_token=None) -> network_models.SubnetList:
        subnet_list = self.__network_client.list_subnets(network_id, page_size=page_size, page_token=page_token)
        subnet_list.subnets = [s for s in subnet_list.subnets if zone_id in {None, s.zone_id} and name in {None, s.name}]
        return subnet_list

    # Subnets

    def list_subnets(self, folder_id, network_id=None, zone_id=None, name=None, page_size=None, page_token=None) -> network_models.SubnetList:
        subnet_list = self.__subnet_client.list(folder_id, filter=self._makefilter(name=name), page_size=page_size, page_token=page_token)
        subnet_list.subnets = [s for s in subnet_list.subnets if zone_id in {None, s.zone_id} and network_id in {None, s.network_id}]
        return subnet_list

    def get_subnet(self, subnet_id) -> network_models.Subnet:
        return self.__subnet_client.get(subnet_id)

    def create_subnet(self, folder_id, network_id, zone_id, name=None, v4_cidr_blocks=None, v6_cidr_blocks=None,
                      description=None, labels=None, route_table_id=None, extra_params=None, egress_nat_enable=None,
                      v4_cidr_block=None, v6_cidr_block=None,
                      **kwargs) -> network_models.SubnetOperation:
        op = self.__subnet_client.create(
            folder_id=folder_id,
            network_id=network_id,
            zone_id=zone_id,
            name=name,
            description=description,
            labels=labels,
            v4_cidr_blocks=v4_cidr_blocks or v4_cidr_block,
            v6_cidr_blocks=v6_cidr_blocks or v6_cidr_block,
            route_table_id=route_table_id,
            egress_nat_enable=egress_nat_enable,
            extra_params=self._to_grpc(extra_params),
        )
        return self.__wait_operation(op, model=network_models.SubnetOperation, **kwargs)

    def update_subnet(self, subnet_id, name=_UNSET, description=_UNSET, labels=_UNSET,
                      route_table_id=_UNSET, export_rts=_UNSET, import_rts=_UNSET, egress_nat_enable=_UNSET,
                      **kwargs) -> network_models.SubnetOperation:
        update = drop_none({
            "name": name,
            "description": description,
            "labels": labels,
            "route_table_id": route_table_id,
            "egress_nat_enable": egress_nat_enable,
        }, none=_UNSET)
        update_mask = list(update)
        op = self.__subnet_client.update(subnet_id, update_mask=update_mask, **update)
        return self.__wait_operation(op, model=network_models.SubnetOperation, **kwargs)

    def delete_subnet(self, subnet_id, **kwargs) -> network_models.SubnetOperation:
        op = self.__subnet_client.delete(subnet_id)
        return self.__wait_operation(op, model=network_models.SubnetOperation, **kwargs)

    def move_subnet(self, subnet_id: str, destination_folder_id: str, **kwargs) -> network_models.SubnetOperation:
        op = self.__subnet_client.move(subnet_id, destination_folder_id)
        return self.__wait_operation(op, model=network_models.SubnetOperation, **kwargs)

    # Route tables

    def create_route_table(self,
                           folder_id,
                           network_id,
                           static_routes,
                           name=None,
                           description=None,
                           labels=None,
                           **kwargs) -> network_models.RouteTableOperation:
        op = self.__route_table_client.create(
            folder_id=folder_id,
            name=name,
            description=description,
            labels=labels,
            network_id=network_id,
            static_routes=self._to_grpc(static_routes),
        )
        return self.__wait_operation(op, model=network_models.RouteTableOperation, **kwargs)

    def update_route_table(self,
                           route_table_id: str,
                           static_routes=_UNSET,
                           name=_UNSET,
                           description=_UNSET,
                           labels=_UNSET,
                           **kwargs) -> network_models.RouteTableOperation:
        update = drop_none({
            "name": name,
            "description": description,
            "labels": labels,
            "static_routes": self._to_grpc(static_routes),
        }, none=_UNSET)
        update_mask = list(update)
        op = self.__route_table_client.update(route_table_id, update_mask=update_mask, **update)
        return self.__wait_operation(op, model=network_models.RouteTableOperation, **kwargs)

    def update_route_table_static_routes(self,
                                         route_table_id: str,
                                         delete: list = None,
                                         upsert: list = None,
                                         **kwargs) -> network_models.RouteTableOperation:
        op = self.__route_table_client.update_static_routes(
            route_table_id=route_table_id,
            delete=self._to_grpc(delete),
            upsert=self._to_grpc(upsert),
        )
        return self.__wait_operation(op, model=network_models.RouteTableOperation, **kwargs)

    def delete_route_table(self, route_table_id: str, **kwargs) -> network_models.RouteTableOperation:
        op = self.__route_table_client.delete(route_table_id)
        return self.__wait_operation(op, model=network_models.RouteTableOperation, **kwargs)

    def move_route_table(self, route_table_id: str, destination_folder_id: str, **kwargs) -> network_models.RouteTableOperation:
        op = self.__route_table_client.move(route_table_id, destination_folder_id)
        return self.__wait_operation(op, model=network_models.RouteTableOperation, **kwargs)

    def list_route_tables(self, folder_id, name=None, page_size=None, page_token=None) -> network_models.RouteTableList:
        return self.__route_table_client.list(folder_id, filter=self._makefilter(name=name), page_size=page_size, page_token=page_token)

    def get_route_table(self, route_table_id: str) -> network_models.RouteTable:
        return self.__route_table_client.get(route_table_id)

    # Addresses

    def list_addresses(self, folder_id, type, name=None, page_size=None, page_token=None) -> network_models.AddressList:
        return self.__address_client.list(folder_id, filter=self._makefilter(name=name, type=type), page_size=page_size, page_token=page_token)

    def get_address(self, address_id) -> network_models.Address:
        return self.__address_client.get(address_id)

    def create_address(self, folder_id, name=None, description=None, labels=None, external_address_spec=None, internal_address_spec=None, ephemeral=None, **kwargs) -> network_models.AddressOperation:
        if sum(map(bool, [external_address_spec, internal_address_spec])) != 1:
            raise YcClientError("Either external_address_spec or internal_address_spec must be specified.")

        external_address_spec = self._to_grpc(external_address_spec)
        internal_address_spec = self._to_grpc(internal_address_spec)

        for spec in [external_address_spec, internal_address_spec]:
            if isinstance(spec, dict) and 'ip_version' in spec:
                spec['ip_version'] = spec['ip_version'].upper()

        op = self.__address_client.create(
            folder_id=folder_id,
            name=name,
            description=description,
            labels=labels,
            external_address_spec=external_address_spec,
            internal_address_spec=internal_address_spec,
            ephemeral=ephemeral,
        )
        return self.__wait_operation(op, model=network_models.AddressOperation, **kwargs)

    def update_address(self, address_id, name=None, description=None, labels=None, ephemeral=None, **kwargs) -> network_models.AddressOperation:
        update = drop_none({
            "name": name,
            "description": description,
            "labels": labels,
            "ephemeral": ephemeral,
        })
        update_mask = list(update)
        op = self.__address_client.update(address_id, update_mask=update_mask, **update)
        return self.__wait_operation(op, model=network_models.AddressOperation, **kwargs)

    def delete_address(self, address_id, **kwargs) -> network_models.AddressOperation:
        op = self.__address_client.delete(address_id)
        return self.__wait_operation(op, model=network_models.AddressOperation, **kwargs)

    # FIP buckets

    def create_fip_bucket(self, folder_id, flavor, scope, cidrs,
                          import_rts=None, export_rts=None, ip_version=None,
                          name=None, description=None, labels=None, **kwargs) -> network_models.FipBucketOperation:
        op = self.__fip_bucket_client.create(
            folder_id=folder_id,
            name=name,
            description=description,
            labels=labels,
            flavor=self._conv_enum(flavor),
            scope=scope,
            cidrs=cidrs,
            import_rts=self._to_grpc(import_rts),
            export_rts=self._to_grpc(export_rts),
            ip_version=self._conv_enum(ip_version),
        )
        return self.__wait_operation(op, model=network_models.FipBucketOperation, **kwargs)

    def delete_fip_bucket(self, bucket_id, **kwargs) -> network_models.FipBucketOperation:
        op = self.__fip_bucket_client.delete(bucket_id)
        return self.__wait_operation(op, model=network_models.FipBucketOperation, **kwargs)

    def list_fip_buckets(self, folder_id) -> network_models.FipBucketList:
        return self.__fip_bucket_client.list(folder_id)

    def get_fip_bucket(self, bucket_id) -> network_models.FipBucket:
        return self.__fip_bucket_client.get(bucket_id)

    # Target groups

    def create_target_group(self, folder_id,
                            targets: List[target_groups_models.Target]=None,
                            name=None, description=None, labels=None, region_id=validation.RegionId.RU_CENTRAL1,
                            **kwargs) -> target_groups_models.TargetGroupOperation:
        op = self.__tg_client.create(
            folder_id=folder_id,
            name=name,
            description=description,
            labels=labels,
            region_id=region_id,
            targets=self._to_grpc(targets),
        )
        return self.__wait_operation(op, model=target_groups_models.TargetGroupOperation, **kwargs)

    def update_target_group(self, target_group_id,
                            targets: List[target_groups_models.Target]=None,
                            name=None, description=None, labels=None, **kwargs) -> target_groups_models.TargetGroupOperation:
        update = drop_none({
            "name": name,
            "description": description,
            "labels": labels,
            "targets": self._to_grpc(targets),
        })
        update_mask = list(update)
        op = self.__tg_client.update(target_group_id, update_mask, **update)
        return self.__wait_operation(op, model=target_groups_models.TargetGroupOperation, **kwargs)

    def delete_target_group(self, target_group_id, **kwargs) -> target_groups_models.TargetGroupOperation:
        op = self.__tg_client.delete(target_group_id)
        return self.__wait_operation(op, model=target_groups_models.TargetGroupOperation, **kwargs)

    def get_target_group(self, target_group_id, **kwargs) -> target_groups_models.TargetGroup:
        return self.__tg_client.get(target_group_id)

    def list_target_groups(self, folder_id,
                           name=None, page_size=None, page_token=None, **kwargs) -> target_groups_models.TargetGroupList:
        return self.__tg_client.list(folder_id, filter=self._makefilter(name=name), page_size=page_size, page_token=page_token)

    def add_target_group_targets(self, target_group_id,
                                 targets: List[target_groups_models.Target],
                                 **kwargs) -> target_groups_models.TargetGroupOperation:
        op = self.__tg_client.add_targets(
            target_group_id=target_group_id,
            targets=self._to_grpc(targets),
        )
        return self.__wait_operation(op, model=target_groups_models.TargetGroupOperation, **kwargs)

    def remove_target_group_targets(self, target_group_id,
                                    targets: List[target_groups_models.Target],
                                    **kwargs) -> target_groups_models.TargetGroupOperation:
        op = self.__tg_client.remove_targets(
            target_group_id=target_group_id,
            targets=self._to_grpc(targets),
        )
        return self.__wait_operation(op, model=target_groups_models.TargetGroupOperation, **kwargs)

    def list_target_group_operations(self, target_group_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__tg_client.list_operations(target_group_id, page_size=page_size, page_token=page_token)

    # Network Load Balancer

    def create_network_load_balancer(self, folder_id, load_balancer_type,
                                     listener_specs: List[network_load_balancers_models.ListenerSpec]=None,
                                     attached_target_groups: List[target_groups_models.AttachedTargetGroupPublic]=None,
                                     name=None, description=None, labels=None, region_id=validation.RegionId.RU_CENTRAL1,
                                     **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        if len(attached_target_groups or []) > 1:
            raise YcClientError("attachedTargetGroups: Please provide no more than 1 item")
        if len(listener_specs or []) > 10:
            raise YcClientError("Please provide no more than 10 items")

        op = self.__lb_client.create(
            folder_id=folder_id,
            type=load_balancer_type.upper(),
            name=name,
            description=description,
            labels=labels,
            region_id=region_id,
            listener_specs=self._to_grpc(listener_specs),
            attached_target_groups=self._to_grpc(attached_target_groups),
        )
        return self.__wait_operation(op, model=network_load_balancers_models.NetworkLoadBalancerOperation, **kwargs)

    def update_network_load_balancer(self, network_load_balancer_id,
                                     listener_specs: List[network_load_balancers_models.ListenerSpec]=None,
                                     attached_target_groups: List[target_groups_models.AttachedTargetGroupPublic]=None,
                                     name=None, description=None, labels=None,
                                     **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        if len(attached_target_groups or []) > 1:
            raise YcClientError("attachedTargetGroups: Please provide no more than 1 item")
        if len(listener_specs or []) > 10:
            raise YcClientError("Please provide no more than 10 items")

        update = drop_none({
            "name": name,
            "description": description,
            "labels": labels,
            "listener_specs": self._to_grpc(listener_specs),
            "attached_target_groups": self._to_grpc(attached_target_groups),
        })
        update_mask = list(update)
        op = self.__lb_client.update(network_load_balancer_id, update_mask=update_mask, **update)
        return self.__wait_operation(op, model=network_load_balancers_models.NetworkLoadBalancerOperation, **kwargs)

    def get_network_load_balancer(self,
                                  network_load_balancer_id,
                                  model=network_load_balancers_models.NetworkLoadBalancerPublic,
                                  **kwargs) -> network_load_balancers_models.NetworkLoadBalancerPublic:
        return self.__lb_client.get(network_load_balancer_id)

    def list_network_load_balancers(self, folder_id,
                                    name=None, page_size=None, page_token=None,
                                    **kwargs) -> network_load_balancers_models.NetworkLoadBalancerList:
        return self.__lb_client.list(folder_id, filter=self._makefilter(name=name), page_size=page_size, page_token=page_token)

    def get_target_states(self, network_load_balancer_id, target_group_id,
                          **kwargs) -> network_load_balancers_models.NetworkLoadBalancerTargetStates:
        return self.__lb_client.get_target_states(network_load_balancer_id, target_group_id)

    def delete_network_load_balancer(self, network_load_balancer_id, **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        op = self.__lb_client.delete(network_load_balancer_id)
        return self.__wait_operation(op, model=network_load_balancers_models.NetworkLoadBalancerOperation, **kwargs)

    def start_network_load_balancer(self, network_load_balancer_id, **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        op = self.__lb_client.start(network_load_balancer_id)
        return self.__wait_operation(op, model=network_load_balancers_models.NetworkLoadBalancerOperation, **kwargs)

    def stop_network_load_balancer(self, network_load_balancer_id, **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        op = self.__lb_client.stop(network_load_balancer_id)
        return self.__wait_operation(op, model=network_load_balancers_models.NetworkLoadBalancerOperation, **kwargs)

    def attach_target_group(self, network_load_balancer_id,
                            target_group: target_groups_models.AttachedTargetGroupPublic,
                            **kwargs) -> network_load_balancers_models.NetworkLoadBalancerAttachmentOperation:
        op = self.__lb_client.attach_target_group(network_load_balancer_id, self._to_grpc(target_group))
        return self.__wait_operation(op, model=network_load_balancers_models.NetworkLoadBalancerAttachmentOperation, **kwargs)

    def detach_target_group(self, network_load_balancer_id, target_group_id,
                            **kwargs) -> network_load_balancers_models.NetworkLoadBalancerAttachmentOperation:
        op = self.__lb_client.detach_target_group(network_load_balancer_id, target_group_id)
        return self.__wait_operation(op, model=network_load_balancers_models.NetworkLoadBalancerAttachmentOperation, **kwargs)

    def add_network_load_balancer_listener(self, network_load_balancer_id,
                                           listener_spec: network_load_balancers_models.ListenerSpecWithoutAntiDdos,
                                           **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        op = self.__lb_client.add_listener(network_load_balancer_id, self._to_grpc(listener_spec))
        return self.__wait_operation(op, model=network_load_balancers_models.NetworkLoadBalancerOperation, **kwargs)

    def remove_network_load_balancer_listener(self, network_load_balancer_id, listener_name, **kwargs) -> network_load_balancers_models.NetworkLoadBalancerOperation:
        op = self.__lb_client.remove_listener(network_load_balancer_id, listener_name)
        return self.__wait_operation(op, model=network_load_balancers_models.NetworkLoadBalancerOperation, **kwargs)

    def list_network_load_balancer_operations(self, nlb_id, page_size=None, page_token=None) -> operations_models.OperationListV1Beta1:
        return self.__lb_client.list_operations(nlb_id, page_size=page_size, page_token=page_token)

    def __wait_operation(self, op, wait_timeout=0, **kwargs):
        return super().wait_operation(op, wait_timeout=wait_timeout, **kwargs)

    @classmethod
    def _to_grpc(cls, v):
        if isinstance(v, Model):
            return v.to_grpc()
        if isinstance(v, list):
            return list(map(cls._to_grpc, v))
        return v

    @classmethod
    def _makefilter(self, **kwargs):
        return " and ".join(" = ".join([k, repr(v)]) for k, v in kwargs.items() if v is not None) or None

    @classmethod
    def _conv_enum(cls, v):
        return None if v is None else v.upper()


class ComputeEndpointConfig(Model):
    url = StringType(required=True)


def get_compute_client(credentials: YandexCloudCredentials, url=None, **kwargs) -> ComputeClient:
    return ComputeClient(
        url if url is not None else config.get_value("endpoints.compute", model=ComputeEndpointConfig).url,
        credentials=credentials,
        **kwargs)


def get_compute_client_with_grpc_vpc(credentials: YandexCloudCredentials, url=None, **kwargs) -> ComputeClient:
    from yc_common.clients.grpc import load_balancer, network, target_group

    return ComputeClientWithGrpcVpc(
        url if url is not None else config.get_value("endpoints.compute", model=ComputeEndpointConfig).url,
        credentials=credentials,
        network_client=network.get_network_client(iam_token=credentials.token),
        route_table_client=network.get_route_table_client(iam_token=credentials.token),
        subnet_client=network.get_subnet_client(iam_token=credentials.token),
        address_client=network.get_address_client(iam_token=credentials.token),
        fip_bucket_client=network.get_fip_bucket_client(iam_token=credentials.token),
        network_load_balancer_client=load_balancer.get_network_load_balancer_client(iam_token=credentials.token),
        target_group_client=target_group.get_target_group_client(iam_token=credentials.token),
        **kwargs)
