#!/usr/bin/env python3
from concurrent import futures
import logging
import grpc
import argparse
import re
import numpy as np
import time
import uuid

from google.protobuf import text_format

from grpc_reflection.v1alpha import reflection
from grpc_health.v1 import health
from grpc_health.v1 import health_pb2
from grpc_health.v1 import health_pb2_grpc

import yandex.cloud.ai.tts.v1.tts_service_pb2 as tts
import yandex.cloud.ai.tts.v1.tts_service_pb2_grpc as tts_grpc

from methods.tacotron_model import SynthesisModel
from tools.audio import AudioSample
from tools.data import sample_sox

one_day_in_sec = 60 * 60 * 24
chunk_size = 8000
word_pattern = re.compile(r'\w+')
min_sample_rate = 22050


def get_audio(request):
    data = np.float32(np.frombuffer(request.template_audio.content, dtype=np.int16)) / np.iinfo(np.int16).max
    if request.template_audio.audio_spec.sample_rate_hertz < min_sample_rate:
        raise Exception("sample rate should be greater then %d" % min_sample_rate)
    return AudioSample(data=data, sample_rate=request.template_audio.audio_spec.sample_rate_hertz)


def get_text(request, context):
    if len(request.template_text) == 0:
        raise Exception("Template text must not be empty")
    text = request.template_text
    word_indexes = list(word_pattern.finditer(request.template_text))
    replacements = sorted(request.replacements, key=lambda replacement: replacement.word_index_start, reverse=True)
    prev_word_index_start = None
    for replacement in replacements:
        if replacement.word_index_start >= len(word_indexes): raise Exception("Word index start is out of range")
        if replacement.word_index_end > len(word_indexes): raise Exception("Word index end is out of range")
        if replacement.word_index_end <= replacement.word_index_start: raise Exception(
            "No words to replace. Word index end have to be higher than word index start")
        if prev_word_index_start and prev_word_index_start < replacement.word_index_end:
            raise Exception("Replacement intersection detected")
        prev_word_index_start = replacement.word_index_start
        start_idx = word_indexes[replacement.word_index_start].span()[0]
        end_idx = word_indexes[replacement.word_index_end - 1].span()[1]
        text = text[:start_idx] + replacement.replacement_text + text[end_idx:]
    return text


class TtsServer(tts_grpc.TtsServiceServicer):
    def __init__(self, model, speaker_id):
        self.__model = model
        self.__speaker_id = speaker_id

    def AdaptiveSynthesize(self, request, context):
        req_id = str(uuid.uuid4())
        logging_extra = {'req_id': req_id}
        sensitive = ['authorization']
        response_code = grpc.StatusCode.OK
        response_details = ''

        def finalize():
            context.set_code(response_code)
            context.set_details(response_details)
            logging.info(f'Request was finished with code: {response_code} and details: {response_details}',
                         extra=logging_extra)

        for key, value in context.invocation_metadata():
            if key not in sensitive:
                logging.info('Received metadata: key=%s value=%s' % (key, value), extra=logging_extra)
            else:
                logging.info('Received metadata: key=%s value=%s' % (key, value[:10] + 'x' * (len(value) - 10)),
                             extra=logging_extra)
        try:
            audio = get_audio(request)

            logging.info('Request text: ' + request.template_text, extra=logging_extra)
            for message in request.replacements:
                logging.info('Replacement: ' + text_format.MessageToString(message, as_utf8=True, as_one_line=True),
                             extra=logging_extra)
            text = get_text(request, context)
        except Exception as e:
            response_code = grpc.StatusCode.INVALID_ARGUMENT
            response_details = str(e)
            finalize()
            return
        try:
            synthesized_audio = self.__model.apply(text, audio, speaker_id=self.__speaker_id)
            if request.output_audio_spec.sample_rate_hertz > 0:
                synthesized_audio = sample_sox([synthesized_audio], request.output_audio_spec.sample_rate_hertz)[0]
            bytes = np.int16(synthesized_audio.data * np.iinfo(np.int16).max).tobytes()
            for i in range(0, len(bytes), chunk_size):
                data_to_send = bytes[i:i + chunk_size]
                data_len = len(data_to_send)
                logging.info(f'Send chunk len={data_len}', extra=logging_extra)
                yield tts.AdaptiveSynthesizeResponse(audio_chunk=tts.AudioChunk(data=data_to_send))
        except Exception as e:
            logging.error('ServerError: ' + str(e))
            response_code = grpc.StatusCode.INTERNAL
            response_details = 'Internal Server Error'
        finalize()


class SynthesisModelStub:
    def apply(self, text, audio, speaker_id):
        return audio


def serve():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vocoder', required=True)
    parser.add_argument('--tacotron', required=True)
    parser.add_argument('--lingware', required=True)
    parser.add_argument('--preprocessor', required=True)
    parser.add_argument('--speaker-id', default='elena_mtt_female')
    params = parser.parse_args()

    model = SynthesisModel(
        waveglow_path=params.vocoder,
        tacotron_path=params.tacotron,
        lingware_path=params.lingware,
        preprocessor_cli_path=params.preprocessor,
    )
    # model = SynthesisModelStub()

    server = grpc.server(futures.ThreadPoolExecutor(max_workers=100))
    tts_grpc.add_TtsServiceServicer_to_server(TtsServer(
        model,
        params.speaker_id,
    ), server)

    # Create a health check servicer. We use the non-blocking implementation
    # to avoid thread starvation.
    health_servicer = health.HealthServicer(
        experimental_non_blocking=True,
        experimental_thread_pool=futures.ThreadPoolExecutor(max_workers=1))
    health_pb2_grpc.add_HealthServicer_to_server(health_servicer, server)

    # Create a tuple of all of the services we want to export via reflection.
    services = tuple(
        service.full_name
        for service in tts.DESCRIPTOR.services_by_name.values()) + (reflection.SERVICE_NAME, health.SERVICE_NAME)

    # Add the reflection service to the server.
    reflection.enable_server_reflection(services, server)
    server.add_insecure_port(f"[::]:80")
    server.start()

    # Mark all services as healthy.
    overall_server_health = ""
    for service in services + (overall_server_health,):
        health_servicer.set(service, health_pb2.HealthCheckResponse.SERVING)

    print('Services ready to handle:', str(services))

    try:
        while True:
            time.sleep(one_day_in_sec)
    except KeyboardInterrupt:
        server.stop(0)


if __name__ == '__main__':
    logging.basicConfig(format="%(asctime)s %(req_id)s : %(message)s", level=logging.INFO)
    serve()
