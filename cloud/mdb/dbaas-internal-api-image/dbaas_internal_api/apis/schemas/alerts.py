# -*- coding: utf-8 -*-
"""
DBaaS Internal API backups schemas
"""

from marshmallow import Schema
from marshmallow.fields import Float, Nested, List, Boolean
import marshmallow.validate

from .common import ListRequestSchemaV1
from .fields import Str


class AlertSchemaV1(Schema):
    templateID = Str(required=True, attribute='template_id')

    notificationChannels = List(
        Str, attribute='notification_channels', requred=True, validate=marshmallow.validate.Length(min=1)
    )
    disabled = Boolean(attibute='disabled', default=False)

    warningThreshold = Float(attribute='warning_threshold')
    criticalThreshold = Float(attribute='critical_threshold')


class AlertTemplateSchemaV1(Schema):
    templateID = Str(required=True, attribute='template_id')

    description = Str(attribute='description', required=True)
    name = Str(attribute='name', required=True)

    warningThreshold = Float(attribute='warning_threshold')
    criticalThreshold = Float(attribute='critical_threshold')

    mandatory = Boolean(attribute='mandatory', required=True)


class AlertTemplateListSchemaV1(Schema):
    alerts = Nested(AlertTemplateSchemaV1, many=True, attribute='alerts')


class AlertGroupSchemaHeaderV1(Schema):
    alerts = Nested(AlertSchemaV1, many=True, attribute='alerts', required=True)
    monitoringFolderId = Str(attribute='monitoring_folder_id', required=True)


class AlertGroupSchemaV1(AlertGroupSchemaHeaderV1):
    alertGroupId = Str(attribute='alert_group_id')


class UpdateAlertGroupRequestSchemaV1(Schema):
    alerts = Nested(AlertSchemaV1, many=True, attribute='alerts')
    monitoringFolderId = Str(attribute='monitoring_folder_id')


class CreateAlertsGroupRequestSchemaV1(AlertGroupSchemaV1):
    """
    Create alert groups in cluster
    """


class GetAlertsGroupRequestSchemaV1(Schema):
    """
    Get cluster alert groups
    """

    alertGroupId = Str(attribute='alert_group_id')


class ListClusterAlertGroupsRequestSchemaV1(ListRequestSchemaV1):
    """
    List alert groups in cluster
    """


class ListAlertGroupsRequestSchemaV1(ListRequestSchemaV1):
    """
    List alert groups request schema
    """
