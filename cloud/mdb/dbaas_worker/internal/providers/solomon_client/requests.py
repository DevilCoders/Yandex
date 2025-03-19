from enum import Enum
from typing import Any, Optional

from dataclasses import dataclass


@dataclass
class UpdateTemplateVersionRequest:
    newTemplateVersionTag: str
    templateId: str


@dataclass
class AlertRequestParam:
    name: str
    value: Any


@dataclass
class RequestAlertFromTemplateType:
    templateId: str
    templateVersionTag: str
    # resourceReference: str = "example.com/resource1"
    doubleValueParameters: Optional[list[AlertRequestParam]] = None
    doubleValueThresholds: Optional[list[AlertRequestParam]] = None
    intValueParameters: Optional[list[AlertRequestParam]] = None
    intValueThresholds: Optional[list[AlertRequestParam]] = None
    labelListValueParameters: Optional[list[AlertRequestParam]] = None
    labelListValueThresholds: Optional[list[AlertRequestParam]] = None
    textListValueParameters: Optional[list[AlertRequestParam]] = None
    textListValueThresholds: Optional[list[AlertRequestParam]] = None
    textValueParameters: Optional[list[AlertRequestParam]] = None
    textValueThresholds: Optional[list[AlertRequestParam]] = None


@dataclass
class RequestType:
    fromTemplate: RequestAlertFromTemplateType


class AlertRequestAlertState(Enum):
    active = 'ACTIVE'


@dataclass
class NotificationChannel:
    id: str
    config: dict


@dataclass
class StubCompleteRequestResource:
    resourceType: str
    resourceParameters: dict
    resourceId: str


@dataclass
class StubCompleteRequest:
    resources: list[StubCompleteRequestResource]


@dataclass
class AlertCreateRequest:
    id: str
    projectId: str
    name: str
    state: str
    channels: list[NotificationChannel]
    description: str
    type: RequestType
    labels: dict
    version: int = -1  # TODO: remove after UI uses Solomon API
