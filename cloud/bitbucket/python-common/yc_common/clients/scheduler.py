"""Scheduler client."""

from typing import List, Optional

from yc_common import fields, logging
from yc_common.misc import drop_none
from yc_common.models import Model, StringType, BooleanType, ModelType, ListType, IntType, DictType

from .api import ApiClient

log = logging.get_logger(__name__)


class SchedulerErrorCodes:
    NotMaster = "NotMaster"
    SchedulerIsInitializing = "SchedulerIsInitializing"

    InvalidInstanceId = "InvalidInstanceId"
    InvalidInstanceState = "InvalidInstanceState"
    InvalidNodeName = "InvalidNodeName"

    ConcurrentInstanceAllocation = "ConcurrentInstanceAllocation"
    NotEnoughResources = "NotEnoughResources"


class SchedulerStatus:
    SLAVE = "slave"
    MASTER = "master"
    BROKEN_MASTER = "broken-master"


class GpuDevicesInfo(Model):
    count = fields.IntType(min_value=0, required=True)
    free = fields.IntType(min_value=0, required=True)


class GpuResourcesInfo(Model):
    cores = fields.IntType(min_value=0, required=True)
    memory = fields.IntType(min_value=0, required=True)


class NumaNodeInfo(Model):
    cores = IntType(required=True)
    free_cores = IntType(required=True)
    preemptible_cores = IntType(required=True)

    shared_cores_limit = IntType(required=True)
    shared_cores_free_compute_units = ListType(StringType, required=True)
    preemptible_shared_cores_compute_units = ListType(StringType, required=True)

    memory = IntType(required=True)
    free_memory = IntType(required=True)
    preemptible_memory = IntType(required=True)

    # FIXME(zasimov-a): mark as required
    gpu_devices = fields.DictType(fields.ModelType(GpuDevicesInfo), key=fields.StringType())
    gpu_resources = fields.ModelType(GpuResourcesInfo, required=False)


class AllocationsInfo(Model):
    total = IntType(required=True)
    preemptible = IntType(required=True)
    preempted = IntType(required=True)


class NodeInfo(Model):
    node_name = StringType(required=True)
    zone_id = StringType(required=True)

    active = BooleanType(required=True)
    enabled = BooleanType(required=True)

    host_groups = ListType(StringType, required=True)
    fault_domain = StringType(required=True)
    hardware_platform = StringType(required=True)
    gpu_hardware_platform = fields.StringType()

    allocations = ModelType(AllocationsInfo, required=True)
    numa_nodes = ListType(ModelType(NumaNodeInfo), required=True)

    nvme_disks = IntType(required=True)
    free_nvme_disks = IntType(required=True)
    preemptible_nvme_disks = IntType(required=True)


class SchedulerClient:
    def __init__(self, name):
        self.__client = ApiClient("http://{host}:7000/v1".format(host=name), timeout=5)

    def get_info(self):
        class Response(Model):
            id = StringType(required=True)
            name = StringType(required=True)
            status = StringType(required=True)
            initialized = BooleanType(required=True)

        return self.__client.get("/info", model=Response)

    def get_nodes_info(self) -> List[NodeInfo]:
        class Response(Model):
            nodes = ListType(ModelType(NodeInfo), required=True)

        return self.__client.get("/info/nodes", model=Response).nodes

    def get_node_info(self, node_name):
        class Response(Model):
            node_name = StringType(required=True)
            enabled = BooleanType(required=True)
            host_groups = ListType(StringType, required=True)

        return self.__client.get("/info/nodes/" + node_name, model=Response)

    def allocate_instance(
        self, zone_id, host_group, platform_id, hardware_platforms, sockets, cores, memory, *,
        instance_id=None, core_fraction=None, gpu_hardware_platforms: Optional[List[str]]=None,
        gpus: Optional[int]=None, nvme_disks=None, placement_group_id=None, nodes=None, exclude_nodes=None,
        preemptible=False, secondary=None, operation_id=None, count=None, dry_run=None
    ):
        class AllocationDetails(Model):
            total_nodes = IntType(required=True)
            matched_nodes = IntType(required=True)
            tried_nodes = IntType(required=True)

            filter_results = DictType(IntType, required=True)
            try_results = DictType(IntType, required=True)

            node_results = DictType(ListType(StringType), required=True)
            allocations = DictType(IntType, required=True)

        class Response(Model):
            if dry_run:
                count = IntType(required=True)
                non_preemptive = ModelType(AllocationDetails, required=True)
                preemptive = ModelType(AllocationDetails)
            else:
                allocation_id = StringType(required=True)
                node_name = StringType(required=True)
                preempted_allocation_ids = ListType(StringType)

        return self.__client.post("/allocate-instance", dict({
            "platform_id": platform_id,
            "hardware_platforms": hardware_platforms,
            "zone_id": zone_id,
            "sockets": sockets,
            "cores": cores,
            "memory": memory,
            "host_group": host_group,
        }, **drop_none({
            "instance_id": instance_id,
            "core_fraction": core_fraction,
            "gpus": gpus,
            # FIXME(zasimov-a): I think gpu_hardware_platforms must be required, move outside of drop_none part.
            "gpu_hardware_platforms": gpu_hardware_platforms,
            "nvme_disks": nvme_disks,
            "placement_group_id": placement_group_id,
            "nodes": nodes,
            "exclude_nodes": exclude_nodes,
            "preemptible": preemptible,
            "secondary": secondary,
            "operation_id": operation_id,
            "count": count,
            "dry_run": dry_run,
        })), model=Response)

    def gc_instance_allocations(self, instance_id):
        return self.__client.post("/instances/{instance_id}/gc".format(instance_id=instance_id), {})

    def get_instance_type_info(self, instance_type_id):
        return self.__client.get("/info/instance_types/" + instance_type_id)

    def delete_node(self, node_name):
        return self.__client.delete("/nodes/" + node_name)

    # Test mode API

    def restart(self):
        return self.__client.post("/restart", {})
