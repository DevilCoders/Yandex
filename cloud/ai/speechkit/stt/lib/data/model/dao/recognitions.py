from datetime import datetime
from enum import Enum
import typing

from cloud.ai.lib.python.datetime import format_datetime
from .records_joins import Recognition
from .records import RecordSourceCloudMethod


class RecognitionSourceType(Enum):
    EVALUATION = 'eval'


class RecognitionSourceEvaluation:
    evaluation_id: str

    def __init__(self, evaluation_id: str):
        self.evaluation_id = evaluation_id

    def to_yson(self) -> typing.Dict:
        return {
            'type': RecognitionSourceType.EVALUATION.value,
            'eval_id': self.evaluation_id,
        }

    @staticmethod
    def from_yson(fields: typing.Dict) -> 'RecognitionSourceEvaluation':
        return RecognitionSourceEvaluation(evaluation_id=fields['eval_id'])


class RecognitionEndpoint:
    api: str
    host: str
    port: int
    method: RecordSourceCloudMethod
    config: typing.Dict

    def __init__(self, api: str, host: str, port: int, method: RecordSourceCloudMethod, config: typing.Dict):
        self.api = api
        self.host = host
        self.port = port
        self.method = method
        self.config = config

    def to_yson(self) -> dict:
        return {
            'api': self.api,
            'host': self.host,
            'port': self.port,
            'method': self.method.to_yson(),
            'config': self.config,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'RecognitionEndpoint':
        return RecognitionEndpoint(
            api=fields['api'],
            host=fields['host'],
            port=fields['port'],
            method=RecordSourceCloudMethod.from_yson(fields['method']),
            config=fields['config'],
        )

    def get_model(self) -> str:
        if self.api == 'google':
            cfg = self.config
            if self.method.name == 'stream':
                cfg = cfg['config']  # recognition spec is wrapped by other config for stream
            return cfg.get('model', '')
        elif self.api == 'tinkoff':
            return ''
        elif self.api == 'stc':
            return self.config['model']
        elif self.api == 'sber':
            return self.config['Options']['model']
        else:
            return self.config['specification'].get('model', '')


class RecognitionElement:
    record_id: str
    recognition: Recognition
    response_chunks: typing.Any
    source: typing.Union[RecognitionSourceEvaluation]
    endpoint: RecognitionEndpoint
    received_at: datetime
    other: typing.Any

    def __init__(
        self,
        record_id: str,
        recognition: Recognition,
        response_chunks: typing.Any,
        source: typing.Union[RecognitionSourceEvaluation],
        endpoint: RecognitionEndpoint,
        received_at: datetime,
        other: typing.Any,
    ):
        self.record_id = record_id
        self.recognition = recognition
        self.response_chunks = response_chunks
        self.source = source
        self.endpoint = endpoint
        self.received_at = received_at
        self.other = other

    def __lt__(self, other: 'RecognitionElement'):
        return self.record_id < other.record_id

    def to_yson(self) -> dict:
        return {
            'record_id': self.record_id,
            'recognition': self.recognition.to_yson(),
            'response_chunks': self.response_chunks,
            'source': self.source.to_yson(),
            'endpoint': self.endpoint.to_yson(),
            'received_at': format_datetime(self.received_at),
            'other': self.other,
        }
