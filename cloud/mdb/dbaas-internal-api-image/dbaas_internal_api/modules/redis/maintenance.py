# -*- coding: utf-8 -*-
"""
DBaaS Internal API Redis maintenance
"""

from datetime import datetime
from typing import Optional

from ...core.types import Operation
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.maintenance import reschedule_maintenance
from ...utils.metadata import RescheduleMaintenanceMetadata
from ...utils.operation_creator import create_finished_operation_for_current_rev
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo, MaintenanceRescheduleType
from .constants import MY_CLUSTER_TYPE
from .traits import RedisOperations


@register_request_handler(MY_CLUSTER_TYPE, Resource.MAINTENANCE, DbaasOperation.RESCHEDULE)
def redis_reschedule_maintenance(
    cluster: ClusterInfo, reschedule_type: MaintenanceRescheduleType, delayed_until: Optional[datetime] = None
) -> Operation:
    """
    Reschedule maintenance task
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_MAINTENANCE_API')
    new_maintenance_time = reschedule_maintenance(cluster, reschedule_type, delayed_until)
    return create_finished_operation_for_current_rev(
        operation_type=RedisOperations.maintenance_reschedule,
        cid=cluster.cid,
        metadata=RescheduleMaintenanceMetadata(new_maintenance_time),
        rev=cluster.rev,
    )
