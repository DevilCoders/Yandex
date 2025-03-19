import io
import sys
import uuid

import grpc

from retry import retry
import yandex.cloud.ai.stt.v2.stt_service_pb2 as stt_service_pb2
import yandex.cloud.ai.stt.v2.stt_service_pb2_grpc as stt_service_pb2_grpc

CHUNK_SIZE = 4000


class Streamer:
    def __init__(self, language: str, api_key: str):
        ##
        #  Создаём grpc канал для отправки запроса на распознавание серверу
        ##
        self.language = language
        self.cred = grpc.ssl_channel_credentials()
        self.channel = grpc.secure_channel('stt.api.cloud.yandex.net:443', self.cred)
        self.stub = stt_service_pb2_grpc.SttServiceStub(self.channel)
        self.api_key = api_key

    @retry(exceptions=(Exception,), tries=100, max_delay=7200, delay=1, backoff=1.1)
    def gen_chunks(self, audio, sample_rate):
        ##
        #  Создаём конфиг, в спецификации указываем тип используемой модели, а также формат передаваемой записи
        ##
        if self.language == 'ru':
            language_code = 'ru-RU'
            model = 'premium'
        elif self.language == 'kk':
            language_code = 'kk-KK'
            model = 'general'
        elif self.language in {"de", "fr"}:
            language_code = 'auto'
            model = 'general:rc'
        else:
            raise ValueError(f'unexpected language: {self.language}')
        specification = stt_service_pb2.RecognitionSpec(
            language_code=language_code,
            profanity_filter=False,
            model=model,
            partial_results=False,
            audio_encoding='LINEAR16_PCM',
            sample_rate_hertz=sample_rate,
            raw_results=True
        )
        streaming_config = stt_service_pb2.RecognitionConfig(specification=specification)

        ##
        #  В качестве первого чанка отправляем конфиг
        ##
        yield stt_service_pb2.StreamingRecognitionRequest(config=streaming_config)

        try:
            ##
            #  Далее отправляем чанки записи без WAV заголовка
            ##
            raw_data = io.BytesIO(audio.data)
            chunk = raw_data.read(CHUNK_SIZE)
            while chunk != b'':
                yield stt_service_pb2.StreamingRecognitionRequest(audio_content=chunk)
                chunk = raw_data.read(CHUNK_SIZE)
        except Exception as e:
            print(f'Failed to load {audio}: {str(e)}', file=sys.stderr)
            raise e

    @retry(exceptions=(Exception,), tries=100, max_delay=7200, delay=1, backoff=1.1)
    def recognize_streaming(self, audio, sample_rate):
        try:
            ##
            #  Отправляем запрос на сервер. В данном случае для получения доступа используется сервисный ключ api_key.
            #  Результатом запроса является стрим (итератор) с промежуточными распознаваниями передаваемой записи.
            ##
            it = self.stub.StreamingRecognize(self.gen_chunks(audio, sample_rate), metadata=(
                ('authorization', 'Api-Key ' + self.api_key),
                ('x-client-request-id', str(uuid.uuid4())),
                ('x-normalize-partials', 'false'),
                ('x-sensitivity-reduction-flag', 'false')
            ))
            return ' '.join(chunk.alternatives[0].text for r in it for chunk in r.chunks)
        except grpc._channel._Rendezvous as err:
            print(f'Failed to recognize {audio}. Error code {err._state.code}, message: {err._state.details}',
                  file=sys.stderr)
            raise err
        except Exception as err:
            print(f'Failed to recognize {audio}: {err}', file=sys.stderr)
            raise err
