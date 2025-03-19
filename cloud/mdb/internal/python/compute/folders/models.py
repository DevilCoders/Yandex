from typing import NamedTuple

from yandex.cloud.priv.resourcemanager.v1 import folder_pb2
from yandex.cloud.priv.access import access_pb2


class ResolvedFolder(NamedTuple):
    id: str
    cloud_id: str

    @staticmethod
    def from_api(raw_folder: folder_pb2.ResolvedFolder) -> 'ResolvedFolder':
        return ResolvedFolder(id=raw_folder.id, cloud_id=raw_folder.cloud_id)


class Subject(NamedTuple):
    id: str
    type: str


class AccessBinding(NamedTuple):
    role_id: str
    subject: Subject

    @staticmethod
    def from_api(raw_binding: access_pb2.AccessBinding) -> 'AccessBinding':
        return AccessBinding(
            role_id=raw_binding.role_id,
            subject=Subject(
                id=raw_binding.subject.id,
                type=raw_binding.subject.type,
            ),
        )
