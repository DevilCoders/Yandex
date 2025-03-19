from typing import Any, Optional

from enum import Enum

from dataclasses import dataclass

from .requests import (
    AlertCreateRequest,
    StubCompleteRequest,
    StubCompleteRequestResource,
    RequestType,
    RequestAlertFromTemplateType,
    AlertRequestParam,
    NotificationChannel,
    AlertRequestAlertState,
    UpdateTemplateVersionRequest,
)


@dataclass
class Param:
    name: str
    value: Any


@dataclass
class Template:
    id: str
    version: str
    doubleValueParameters: Optional[list[Param]] = None
    doubleValueThresholds: Optional[list[Param]] = None
    intValueParameters: Optional[list[Param]] = None
    intValueThresholds: Optional[list[Param]] = None
    labelListValueParameters: Optional[list[Param]] = None
    labelListValueThresholds: Optional[list[Param]] = None
    textListValueParameters: Optional[list[Param]] = None
    textListValueThresholds: Optional[list[Param]] = None
    textValueParameters: Optional[list[Param]] = None
    textValueThresholds: Optional[list[Param]] = None


class AlertState(Enum):
    creating = 'CREATING'
    active = 'ACTIVE'
    updating = 'UPDATING'
    deleting = 'DELETING'
    create_error = 'CREATE-ERROR'
    delete_error = 'DELETE-ERROR'


@dataclass
class Alert:
    ext_id: str
    project_id: str
    name: str
    state: AlertState
    template: Template
    notification_channels: list[str]
    description: str
    alert_group_id: str

    def __hash__(self):
        return hash(f'{self.alert_group_id}-{self.template.id}')

    def __repr__(self):
        ext_id = self.ext_id
        template_id = self.template.id
        alert_group_id = self.alert_group_id
        template_version = self.template.version
        return f'{self.__class__.__name__} {ext_id=}: ({template_id=},{template_version=} {alert_group_id=})'


def req_param_from_params(ps: list[Param]) -> list[AlertRequestParam]:
    return [AlertRequestParam(name=param.name, value=param.value) for param in ps]


def req_update_template_version_from_alert(alert: Alert, cid: str) -> UpdateTemplateVersionRequest:
    return UpdateTemplateVersionRequest(
        newTemplateVersionTag=alert.template.version,
        templateId=alert.template.id,
    )


def req_from_alert(alert: Alert, cid: str) -> AlertCreateRequest:
    return AlertCreateRequest(
        id=alert.ext_id,
        projectId=alert.project_id,
        name=alert.name,
        state=AlertRequestAlertState.active.value,
        channels=[
            NotificationChannel(
                id=el,
                config={
                    "notifyAboutStatuses": [
                        "OK",
                        "WARN",
                        "ALARM",
                    ],
                    "repeatDelaySecs": 0,
                },
            )
            for el in alert.notification_channels
        ],
        labels={
            "cid": cid,
        },
        description=alert.description,
        type=RequestType(
            fromTemplate=RequestAlertFromTemplateType(
                templateId=alert.template.id,
                templateVersionTag=alert.template.version,
                doubleValueParameters=req_param_from_params(alert.template.doubleValueParameters or []),
                doubleValueThresholds=req_param_from_params(alert.template.doubleValueThresholds or []),
                intValueParameters=req_param_from_params(alert.template.intValueParameters or []),
                intValueThresholds=req_param_from_params(alert.template.intValueThresholds or []),
                labelListValueParameters=req_param_from_params(alert.template.labelListValueParameters or []),
                labelListValueThresholds=req_param_from_params(alert.template.labelListValueThresholds or []),
                textValueParameters=req_param_from_params(alert.template.textValueParameters or []),
                textValueThresholds=req_param_from_params(alert.template.textValueThresholds or []),
                textListValueParameters=req_param_from_params(alert.template.textListValueParameters or []),
                textListValueThresholds=req_param_from_params(alert.template.textListValueThresholds or []),
            )
        ),
    )


def req_stub_complete_request(cid: str) -> StubCompleteRequest:
    return StubCompleteRequest(
        resources=[
            StubCompleteRequestResource(
                resourceType="cluster",
                resourceParameters={
                    "cluster": cid,
                },
                resourceId=cid,
            ),
        ],
    )
