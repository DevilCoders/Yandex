import abc
import json
import typing
from dataclasses import dataclass
from datetime import datetime
from enum import Enum

import toloka.client as toloka

from cloud.ai.lib.python.datetime import now
import cloud.ai.lib.python.serialization as serialization


def get_toloka_client_by_nirvana_params(nv_context: typing.Any) -> toloka.TolokaClient:
    return toloka.TolokaClient(
        token=nv_context.parameters.get('toloka-token'),
        environment=toloka.TolokaClient.Environment[nv_context.parameters.get('toloka-environment')],
        retries=100,
        timeout=(60.0, 240.0))


@dataclass
class TolokaObject(serialization.YsonSerializable):
    data: dict

    def __init__(self, data: typing.Union[toloka.primitives.base.BaseTolokaObject, dict]):
        if not isinstance(data, dict):
            data = data.unstructure()
        self.data = data

    def get_data(self) -> toloka.primitives.base.BaseTolokaObject:
        return toloka.structure(self.data, self.get_class())

    @abc.abstractmethod
    def get_class(self) -> type:
        pass


class Pool(TolokaObject):
    def get_class(self) -> type:
        return toloka.Pool


@dataclass
class S3Object(serialization.YsonSerializable):
    endpoint: str
    bucket: str
    key: str

    def to_https_url(self) -> str:
        return 'https://{endpoint}/{bucket}/{key}'.format(
            endpoint=self.endpoint,
            bucket=self.bucket,
            key=self.key,
        )

    @staticmethod
    def from_https_url(url) -> 'S3Object':
        url_without_scheme = url[len('https://'):]
        endpoint, bucket, key = url_without_scheme.split('/', 2)
        return S3Object(endpoint=endpoint, bucket=bucket, key=key)


class ExecutionPlatform(Enum):
    NIRVANA = 'nirvana'


@dataclass
class WhiteboardMeta(serialization.OrderedYsonSerializable):
    id: str
    data_s3_obj: S3Object
    cls: str
    source: ExecutionPlatform
    source_data: dict
    received_at: datetime

    @staticmethod
    def primary_key() -> list[str]:
        return ['id']


whiteboard_classes_registry: dict[str, serialization.YsonSerializable] = {}


def register_whiteboard_class(obj_class: typing.ClassVar[serialization.YsonSerializable]):
    global whiteboard_classes_registry
    whiteboard_classes_registry[obj_class.__name__] = obj_class


def create_whiteboard_meta_from_nirvana_workflow(
    id: str,
    cls: str,  # typing.ClassVar[serialization.YsonSerializable] # quick & dirty for Nirvana ops simplicity
    nv_context: typing.Any,
    s3_dir_path: S3Object,
) -> WhiteboardMeta:
    s3_obj = S3Object(
        endpoint=s3_dir_path.endpoint,
        bucket=s3_dir_path.bucket,
        key=f'{s3_dir_path.key}/{id}',
    )
    return WhiteboardMeta(
        id=id,
        data_s3_obj=s3_obj,
        cls=cls,
        source=ExecutionPlatform.NIRVANA,
        source_data={'workflow_url': nv_context.get_meta().workflow_url},
        received_at=now(),
    )


def materialize_objects_to_nirvana_output(
    objects: list[serialization.YsonSerializable],
    nv_context: ...,
    output_name: str,
):
    materialize_dicts_to_nirvana_output([obj.to_yson() for obj in objects], nv_context, output_name)


def materialize_dicts_to_nirvana_output(
    dicts: list[dict],
    nv_context: ...,
    output_name: str,
):
    with open(nv_context.outputs.get(output_name), 'w') as f:
        json.dump(dicts, f, indent=4, ensure_ascii=False)


def materialize_object_to_nirvana_output(
    obj: serialization.YsonSerializable,
    nv_context: typing.Any,
    output_name: str,
):
    materialize_dict_to_nirvana_output({} if obj is None else obj.to_yson(), nv_context, output_name)


def materialize_dict_to_nirvana_output(
    dct: dict,
    nv_context: typing.Any,
    output_name: str,
):
    with open(nv_context.outputs.get(output_name), 'w') as f:
        json.dump(dct, f, indent=4, ensure_ascii=False)


def dematerialize_objects_from_nirvana_input(
    obj_cls: typing.ClassVar[serialization.YsonSerializable],
    nv_context: typing.Any,
    input_name: str,
) -> list[serialization.YsonSerializable]:
    return [obj_cls.from_yson(dct) for dct in dematerialize_dicts_from_nirvana_input(nv_context, input_name)]


def dematerialize_dicts_from_nirvana_input(
    nv_context: typing.Any,
    input_name: str,
) -> list[dict]:
    with open(nv_context.inputs.get(input_name)) as f:
        return json.load(f)


def dematerialize_object_from_nirvana_input(
    obj_cls: typing.ClassVar[serialization.YsonSerializable],
    nv_context: typing.Any,
    input_name: str,
) -> typing.Optional[serialization.YsonSerializable]:
    dct = dematerialize_dict_from_nirvana_input(nv_context, input_name)
    if dct:
        return obj_cls.from_yson(dct)
    return None


def dematerialize_dict_from_nirvana_input(
    nv_context: typing.Any,
    input_name: str,
) -> dict:
    with open(nv_context.inputs.get(input_name)) as f:
        return json.load(f)
