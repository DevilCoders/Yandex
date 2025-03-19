#!/usr/bin/python3

import typing

import nirvana.job_context as nv
import ujson as json

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    RecognitionElement,
    RecognitionEndpoint,
    RecognitionSourceEvaluation,
    RecognitionPlainTranscript,
    MultiChannelRecognition,
    ChannelRecognition,
)
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_recognitions_meta


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    params = op_ctx.parameters

    enabled: bool = params.get('enabled')
    if not enabled:
        print('Storing is disabled')
        return

    recognitions_path: str = inputs.get('recognitions.json')
    response_chunks_path: str = inputs.get('response_chunks.json')
    recognition_params_path: str = inputs.get('params_of_recognition.json')

    with open(recognitions_path) as f:
        record_id_to_recognition: typing.Dict[str, typing.List[str]] = json.load(f)

    with open(response_chunks_path) as f:
        record_id_to_response_chunks: typing.Dict[str, typing.Any] = json.load(f)

    assert record_id_to_recognition.keys() == record_id_to_response_chunks.keys()

    with open(recognition_params_path) as f:
        recognition_params = json.load(f)

    source = RecognitionSourceEvaluation(evaluation_id=recognition_params['eval_id'])
    endpoint = RecognitionEndpoint.from_yson(recognition_params)

    received_at = now()
    recognitions = []

    for record_id, channel_texts in record_id_to_recognition.items():
        recognition = get_recognition_for_channel_texts(channel_texts)
        response_chunks = record_id_to_response_chunks[record_id]
        recognitions.append(
            RecognitionElement(
                record_id=record_id,
                recognition=recognition,
                response_chunks=response_chunks,
                source=source,
                endpoint=endpoint,
                received_at=received_at,
                other=None,
            )
        )

    table_recognitions = Table(meta=table_recognitions_meta, name=Table.get_name(received_at))
    table_recognitions.append_objects(recognitions)


# TODO: reuse code from create record joins module
def get_recognition_for_channel_texts(texts: typing.List[str]):
    recognition_channels = []
    for i, text in enumerate(texts):
        recognition_channels.append(ChannelRecognition(
            channel=i + 1,
            recognition=RecognitionPlainTranscript(text=text),
        ))
    if len(texts) == 1:
        # Simplify data structure
        return recognition_channels[0].recognition
    return MultiChannelRecognition(channels=recognition_channels)
