import grpc
import uuid
from typing import Optional, List, Tuple
from pydub import AudioSegment

from yandex.cloud.ai.stt.v3 import stt_pb2, stt_service_pb2_grpc

from speechkit.common.utils import get_yandex_credentials
from .. import AudioProcessingType, RecognitionConfig, RecognitionModel
from .. import Transcription, Word


class YandexRecognizer(RecognitionModel):
    def __init__(self,
                 endpoint: str = 'stt.api.cloud.yandex.net:443',
                 use_ssl: bool = True,
                 service_branch: Optional[Tuple[str, str]] = None,
                 **kwargs):
        super().__init__()

        self._service_branch = service_branch  # key-value header

        opts = [('grpc.max_message_length', 128 * 1024 * 1024)]
        if use_ssl:
            cred = grpc.ssl_channel_credentials()
            self._channel = grpc.secure_channel(endpoint, cred, options=opts)
        else:
            self._channel = grpc.insecure_channel(endpoint, options=opts)

        while True:
            try:
                grpc.channel_ready_future(self._channel).result(timeout=10)
                break
            except grpc.FutureTimeoutError:
                print('Recognition service is temporarily unavailable')

    @staticmethod
    def __gen_requests(pcm: bytes, sample_rate: int, sample_width: int, recognition_config: RecognitionConfig):
        if recognition_config.language is not None:
            language_restriction = stt_pb2.LanguageRestrictionOptions(
                restriction_type=stt_pb2.LanguageRestrictionOptions.WHITELIST,
                language_code=[recognition_config.language]
            )
        else:
            language_restriction = stt_pb2.LanguageRestrictionOptions(
                restriction_type=stt_pb2.LanguageRestrictionOptions.LANGUAGE_RESTRICTION_TYPE_UNSPECIFIED
            )

        if recognition_config.mode == AudioProcessingType.Stream:
            audio_processing_type = stt_pb2.RecognitionModelOptions.REAL_TIME
        else:
            audio_processing_type = stt_pb2.RecognitionModelOptions.FULL_DATA

        recognize_options = stt_pb2.StreamingOptions(
            recognition_model=stt_pb2.RecognitionModelOptions(
                audio_format=stt_pb2.AudioFormatOptions(
                    raw_audio=stt_pb2.RawAudio(
                        audio_encoding=stt_pb2.RawAudio.LINEAR16_PCM,
                        sample_rate_hertz=sample_rate,
                        audio_channel_count=1
                    )
                ),
                text_normalization=stt_pb2.TextNormalizationOptions(
                    text_normalization=stt_pb2.TextNormalizationOptions.TEXT_NORMALIZATION_ENABLED,
                    profanity_filter=False,
                    literature_text=True
                ),
                language_restriction=language_restriction,
                audio_processing_type=audio_processing_type
            )
        )

        yield stt_pb2.StreamingRequest(session_options=recognize_options)

        chunk_duration = 0.2
        chunk_size = int(sample_rate * sample_width * chunk_duration)

        for i in range(0, len(pcm), chunk_size):
            yield stt_pb2.StreamingRequest(chunk=stt_pb2.AudioChunk(data=pcm[i:i+chunk_size]))

    def __transcribe(self, channel: int, pcm: bytes, sample_rate: int, sample_width: int, recognition_config: RecognitionConfig) -> Transcription:
        assert sample_width == 2

        req_id = str(uuid.uuid4())
        stub = stt_service_pb2_grpc.RecognizerStub(self._channel)

        try:
            metadata = [
                ('authorization', get_yandex_credentials()),
                ('x-client-request-id', req_id),
            ]
            if self._service_branch is not None:
                metadata.append(self._service_branch)

            it = stub.RecognizeStreaming(self.__gen_requests(pcm, sample_rate, sample_width, recognition_config), metadata=metadata)

            raw_recognitions, normalized_recognitions, words = [], [], []
            for r in it:
                if r.HasField('final'):
                    if len(r.final.alternatives) != 0:
                        raw_recognitions.append(r.final.alternatives[0].text)
                        for word in r.final.alternatives[0].words:
                            words.append(Word(word.text, word.start_time_ms, word.end_time_ms))
                if r.HasField('final_refinement'):
                    alternatives = r.final_refinement.normalized_text.alternatives
                    if len(alternatives) != 0:
                        normalized_recognitions.append(alternatives[0].text)
            return Transcription(
                raw_text=' '.join(raw_recognitions),
                normalized_text=' '.join(normalized_recognitions),
                words=words,
                channel=str(channel)
            )
        except grpc._channel._Rendezvous as err:
            print(f'Failed to recognize audio, request_id={req_id}. Error code {err._state.code}, message: {err._state.details}')
            raise err
        except Exception as err:
            print(f'Failed to recognize audio, request_id={req_id}. Error: {err}')
            raise err

    def transcribe(self, audio: AudioSegment, recognition_config: RecognitionConfig = None) -> List[Transcription]:
        if recognition_config is None:
            recognition_config = RecognitionConfig()

        transcriptions = []
        for channel, content in enumerate(audio.split_to_mono()):
            transcriptions.append(self.__transcribe(channel, content.raw_data, audio.frame_rate, audio.sample_width, recognition_config))
        return transcriptions
