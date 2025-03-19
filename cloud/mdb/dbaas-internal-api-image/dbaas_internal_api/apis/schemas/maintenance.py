# -*- coding: utf-8 -*-
"""
DBaaS Internal API maintenance schema
"""
from marshmallow import Schema

from ...apis.schemas.fields import MappedEnum
from ...utils.types import MaintenanceRescheduleType
from ...utils.validation import GrpcTimestamp


class RescheduleMaintenanceRequestSchemaV1(Schema):
    rescheduleType = MappedEnum(
        {
            'RESCHEDULE_TYPE_UNSPECIFIED': MaintenanceRescheduleType.unspecified,
            'IMMEDIATE': MaintenanceRescheduleType.immediate,
            'NEXT_AVAILABLE_WINDOW': MaintenanceRescheduleType.next_available_window,
            'SPECIFIC_TIME': MaintenanceRescheduleType.specific_time,
        },
        attribute='reschedule_type',
        required=True,
    )

    delayedUntil = GrpcTimestamp(attribute='delayed_until')
