from abc import ABC, abstractmethod
from pymorphy2 import MorphAnalyzer

import grpc
from .normalizer_service import normalizer_pb2
from .normalizer_service import normalizer_pb2_grpc


class TextTransformer(ABC):
    @abstractmethod
    def transform(self, text: str) -> str:
        raise NotImplementedError


class Lemmatizer(TextTransformer):
    morphy: MorphAnalyzer

    def __init__(self):
        self.morphy = MorphAnalyzer()

    def transform(self, text: str) -> str:
        """
        Transforms texts, bringing all its words to their initial forms

        :param text: str
            Initial text.

        :return: Transformed text.
        """
        tokens = text.split(' ')
        return ' '.join([
            self.morphy.parse(token)[0].normal_form for token in tokens
        ])


class Normalizer(TextTransformer):
    def __init__(self):
        channel_creds = grpc.ssl_channel_credentials()
        service_address = 'stt-metrics.api.cloud.yandex.net:443'
        self._channel = grpc.secure_channel(service_address, channel_creds)
        self._retries = 3

    def transform(self, text: str) -> str:
        """
        Transforms texts, replacing word representations of numbers and abbreviations with normalized ones.

        :param text: str
            Initial text.

        :return: Transformed text.
        """
        status = grpc.StatusCode.UNKNOWN
        for i in range(self._retries):
            try:
                stub = normalizer_pb2_grpc.NormalizerStub(self._channel)
                result = stub.NormalizeText(normalizer_pb2.Request(text=text))
                return result.text
            except grpc.RpcError as e:
                status = e.code()
                if status != grpc.StatusCode.UNAVAILABLE:
                    break
        raise ConnectionError(f'Normalizer service is not available, status code: {status}. '
                              f'Please, try again or contact us via Support: https://console.cloud.yandex.ru/support')

    def __del__(self):
        self._channel.close()
