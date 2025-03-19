from dataclasses import dataclass
from datetime import datetime
import random
import typing

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Mark,
    S3Object,
    HashVersion,
    RecordSourceCloud,
    RecordSourceCloudMethod,
    Record,
    RecordAudio,
    RecordTag,
    RecordRequestParams,
    RecordAudioParams,
    RecordTagType,
    RecordTagData,
    AudioEncoding,
)
from cloud.ai.speechkit.stt.lib.data.model.registry.tags import folder_id_to_client_slug


@dataclass
class RecordData:
    """
    Record data either from proxy logs or from some import data file.
    """

    id: str
    s3_obj: S3Object
    hash: bytes
    hash_version: HashVersion
    req_params: RecordRequestParams
    duration_seconds: float
    audio_channel_count: typing.Optional[int]
    size_bytes: int
    folder_id: str
    method: str
    received_at: datetime


def assign_mark(train_size=0.8, test_size=0.1) -> Mark:
    assert train_size >= 0.0 and test_size >= 0.0 and train_size + test_size <= 1.0
    # TRAIN/TEST/VAL = train_size/test_size/1 - (test_size + train_size)
    r = random.uniform(0, 1)
    if r < train_size:
        return Mark.TRAIN
    elif r < train_size + test_size:
        return Mark.VAL
    else:
        return Mark.TEST


def create_record_from_data(record_data: RecordData) -> typing.Tuple[Record, RecordAudio, typing.List[RecordTag]]:
    method_and_version = {
        'speechkit.stt': ('short', 'v1'),
        'speechkit.stt_stream': ('stream', 'v2'),
        'speechkit.stt_long_running': ('long', 'v2'),
    }.get(record_data.method)

    if method_and_version is None:
        raise ValueError(f'Unknown method {record_data.method}')

    method, version = method_and_version

    record_data.req_params.fix_lang()

    if record_data.audio_channel_count is None:
        # Cases than channel count absence is ok:
        # - short and stream methods, they are currently works only with 1-channel audio,
        # - LPCM record, it is okay to not specify channels count in recognition spec, default is 1.
        # For OGG OPUS records and long method, channels count must be calculated in services proxy.
        if method == 'long' and record_data.req_params.get_audio_encoding() == AudioEncoding.OGG_OPUS:
            raise ValueError(f'Record {record_data.id} has no information about channel count '
                             f'and default channel count not applicable')
        record_data.audio_channel_count = 1

    record = Record(
        id=record_data.id,
        s3_obj=record_data.s3_obj,
        mark=assign_mark(),
        source=RecordSourceCloud(
            folder_id=record_data.folder_id,
            method=RecordSourceCloudMethod(
                name=method,
                version=version,
            ),
        ),
        req_params=record_data.req_params,
        audio_params=RecordAudioParams(
            acoustic='unknown',
            duration_seconds=record_data.duration_seconds,
            size_bytes=record_data.size_bytes,
            channel_count=record_data.audio_channel_count,
        ),
        received_at=record_data.received_at,
        other=None,
    )
    record_audio = RecordAudio(
        record_id=record_data.id, audio=None, hash=record_data.hash, hash_version=record_data.hash_version
    )

    client_slug = folder_id_to_client_slug.get(record_data.folder_id)

    record_tags = []
    if client_slug is not None:
        record_tags.append(
            RecordTag.add(
                record=record,
                data=RecordTagData(type=RecordTagType.CLIENT, value=client_slug),
                received_at=record.received_at,
            )
        )

    record_tags.append(
        RecordTag.add(
            record=record,
            data=RecordTagData.create_period(record.received_at),
            received_at=record.received_at,
        )
    )
    record_tags.append(
        RecordTag.add(
            record=record,
            data=RecordTagData(type=RecordTagType.MODE, value=method),
            received_at=record.received_at,
        )
    )
    record_tags.append(
        RecordTag.add(
            record=record,
            data=RecordTagData.create_lang(record.req_params.get_language_code()),
            received_at=record.received_at,
        )
    )

    return record, record_audio, record_tags
