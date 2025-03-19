"""
Common maintenance operations
"""

from datetime import datetime
from typing import Optional

from ..core.exceptions import DbaasClientError, NoChangesError
from .time import now
from ..utils import metadb
from ..utils.types import ClusterInfo, MaintenanceRescheduleType
from ..utils.maintenance_helpers import calculate_nearest_maintenance_window


class MaxDelayExceeded(DbaasClientError):
    pass


def reschedule_maintenance(
    cluster: ClusterInfo,
    reschedule_type: MaintenanceRescheduleType,
    delayed_until: Optional[datetime],
) -> datetime:
    if cluster.planned_operation is None:
        raise DbaasClientError('There is no maintenance operation at this time')

    new_maintenance_time = cluster.planned_operation.delayed_until
    if reschedule_type == MaintenanceRescheduleType.unspecified:
        raise DbaasClientError('Reschedule type need to be specified')
    elif reschedule_type == MaintenanceRescheduleType.immediate:
        new_maintenance_time = now()
    elif reschedule_type == MaintenanceRescheduleType.specific_time:
        if delayed_until is None:
            raise DbaasClientError('Delayed until time need to be specified with specific time rescheduling type')
        new_maintenance_time = delayed_until
    elif reschedule_type == MaintenanceRescheduleType.next_available_window:
        if cluster.maintenance_window.is_anytime:
            raise DbaasClientError('The window need to be specified with next available window rescheduling type')
        new_maintenance_time = calculate_nearest_maintenance_window(
            cluster.planned_operation.delayed_until,
            cluster.maintenance_window.weekly_maintenance_window.day,  # type: ignore
            cluster.maintenance_window.weekly_maintenance_window.hour,  # type: ignore
        )
    if new_maintenance_time == cluster.planned_operation.delayed_until:
        raise NoChangesError()

    if (
        new_maintenance_time > cluster.planned_operation.latest_maintenance_time
        and reschedule_type != MaintenanceRescheduleType.immediate
    ):
        raise MaxDelayExceeded(
            'Maintenance operation time cannot exceed {max_delay}'.format(
                max_delay=cluster.planned_operation.latest_maintenance_time
            )
        )

    metadb.reschedule_maintenance_task(cluster.cid, cluster.planned_operation.config_id, new_maintenance_time)
    return new_maintenance_time
