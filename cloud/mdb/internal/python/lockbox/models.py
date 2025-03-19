from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from typing import NamedTuple, Union, List, Dict, Optional

from yandex.cloud.priv.lockbox.v1 import (
    secret_pb2,
    payload_pb2,
)

from cloud.mdb.internal.python.compute.models import enum_from_api


class SecretStatus(Enum):
    STATUS_UNSPECIFIED = secret_pb2.Secret.Status.STATUS_UNSPECIFIED
    CREATING = secret_pb2.Secret.Status.CREATING
    ACTIVE = secret_pb2.Secret.Status.ACTIVE
    INACTIVE = secret_pb2.Secret.Status.INACTIVE
    DELETED = secret_pb2.Secret.Status.DELETED

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, SecretStatus)


class VersionStatus(Enum):
    STATUS_UNSPECIFIED = secret_pb2.Version.Status.STATUS_UNSPECIFIED
    ACTIVE = secret_pb2.Version.Status.ACTIVE
    SCHEDULED_FOR_DESTRUCTION = secret_pb2.Version.Status.SCHEDULED_FOR_DESTRUCTION
    DESTROYED = secret_pb2.Version.Status.DESTROYED

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, VersionStatus)


class PayloadEntryType(Enum):
    PAYLOAD_ENTRY_TYPE_UNSPECIFIED = secret_pb2.Version.PayloadEntryType.PAYLOAD_ENTRY_TYPE_UNSPECIFIED
    TEXT = secret_pb2.Version.PayloadEntryType.TEXT
    BINARY = secret_pb2.Version.PayloadEntryType.BINARY

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, PayloadEntryType)


class Version(NamedTuple):
    id: str
    secret_id: str
    created_at: datetime
    destroy_at: datetime
    description: str
    status: VersionStatus
    payload_entry_keys: List[str]
    payload_entry_types: List[PayloadEntryType]
    _grpc_model = secret_pb2.Version

    @staticmethod
    def from_api(grpc_response: secret_pb2.Version):
        return Version(
            id=grpc_response.id,
            secret_id=grpc_response.secret_id,
            created_at=grpc_response.created_at.ToDatetime(),
            destroy_at=grpc_response.destroy_at.ToDatetime(),
            description=grpc_response.description,
            status=VersionStatus.from_api(grpc_response.status),
            payload_entry_keys=grpc_response.payload_entry_keys,
            payload_entry_types=[
                PayloadEntryType.from_api(payload_entry_type)
                for payload_entry_type in grpc_response.payload_entry_types
            ],
        )


class Secret(NamedTuple):
    id: str
    folder_id: str
    created_at: datetime
    name: str
    description: str
    labels: Dict[str, str]
    kms_key_id: str
    status: SecretStatus
    current_version: Version
    deletion_protection: bool
    _grpc_model = secret_pb2.Secret

    @staticmethod
    def from_api(grpc_response: secret_pb2.Secret):
        return Secret(
            id=grpc_response.id,
            folder_id=grpc_response.folder_id,
            created_at=grpc_response.created_at.ToDatetime(),
            name=grpc_response.name,
            description=grpc_response.description,
            labels=grpc_response.labels,
            kms_key_id=grpc_response.kms_key_id,
            status=SecretStatus.from_api(grpc_response.status),
            current_version=Version.from_api(grpc_response.current_version),
            deletion_protection=grpc_response.deletion_protection,
        )


class Entry(NamedTuple):
    key: str
    value: Union[str, bytes]

    @staticmethod
    def from_api(grpc_response: payload_pb2.Payload.Entry):
        if hasattr(grpc_response, 'text_value'):
            value = grpc_response.text_value
        elif hasattr(grpc_response, 'binary_value'):
            value = grpc_response.binary_value
        else:
            raise TypeError('value must be either text_value or binary_value')

        return Entry(
            key=grpc_response.key,
            value=value,
        )


class Payload(NamedTuple):
    version_id: str
    entries: List[Entry]

    @staticmethod
    def from_api(grpc_response: payload_pb2.Payload):
        return Payload(
            version_id=grpc_response.version_id,
            entries=[Entry.from_api(entry) for entry in grpc_response.entries],
        )


@dataclass
class PayloadEntryChange:
    key: str
    text_value: Optional[str] = None
    binary_value: Optional[bytes] = None
    reference: Optional[str] = None
