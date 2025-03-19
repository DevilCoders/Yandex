import datetime
import json
import logging
import random
import sys
import typing as tp

from nope import JobProcessorOperation
from nope.endpoints import *
from nope.parameters import *
import yt.wrapper as yt

logging.basicConfig(stream=sys.stdout,
                    format="%(levelname)s: %(asctime)s %(filename)s:%(lineno)d     %(message)s")
logger = logging.getLogger()


class GenerateTextsSet(JobProcessorOperation):
    name = 'GenerateTextsSet'
    owner = 'cloud-ml'
    description = 'Generate texts set for TTS'

    def __init__(self):
        super().__init__()
        self.sources = {
            'ru': [
                '//home/mlcloud/tts/text_sets/ru/short_texts/latest',
                '//home/mlcloud/tts/text_sets/ru/questions/latest',
                '//home/mlcloud/tts/text_sets/ru/long_texts/latest',
                '//home/mlcloud/tts/text_sets/ru/templates/latest',
                '//home/mlcloud/tts/text_sets/ru/base/latest'
            ],
            'kz': [
                '//home/mlcloud/tts/text_sets/kz/base/latest'
            ],
            'de': [
                '//home/mlcloud/tts/text_sets/de/base/latest'
            ],
            'fr': [
                '//home/mlcloud/tts/text_sets/fr/base/latest'
            ],
            'he': [
                '//home/mlcloud/tts/text_sets/he/base/latest'
            ]
        }

    class Parameters(JobProcessorOperation.Parameters):
        folder_id = StringParameter(nirvana_name='folder-id', required=True)
        language = EnumParameter(nirvana_name='language', default_value='ru',
                                 enum_values=[
                                     ('ru', 'ru'),
                                     ('kz', 'kz'),
                                     ('de', 'de'),
                                     ('fr', 'fr'),
                                     ('he', 'he')
                                 ])
        offset = IntegerParameter(nirvana_name='offset', required=True)
        texts_number = IntegerParameter(nirvana_name='texts-number', required=True)
        yt_token = SecretParameter(nirvana_name='yt-token', required=True)

    class Outputs(JobProcessorOperation.Outputs):
        texts = TSVOutput(nirvana_name='texts', create_empty_on_exit_if_missing=True)
        log_table = MRTableOutput(nirvana_name='log_table')

    def _write_to_yt(self, result: tp.List[dict], table_path: str, schema: tp.List[dict]) -> str:
        self.yt_client.create('table', table_path, recursive=True, attributes={'schema': schema})
        self.yt_client.write_table(table_path, result, raw=False)
        return table_path

    def _get_texts_set(self, language: str) -> tp.List[dict]:
        texts_set = []
        for source in self.sources[language]:
            texts = [row for row in self.yt_client.read_table(source)]
            random.shuffle(texts)
            texts_set.extend(texts)
        return texts_set

    def run(self):
        if self.Parameters.offset < 0:
            logger.error("'offset' must be a nonnegative number")
            return

        if self.Parameters.texts_number <= 0:
            logger.error("'texts_number' must be a positive number")
            return

        random.seed(self.Parameters.folder_id)

        schema = [{'name': 'uuid', 'type': 'string'},
                  {'name': 'text', 'type': 'utf8'},
                  {'name': 'split', 'type': 'utf8'},
                  {'name': 'source', 'type': 'utf8'}]

        self.yt_client = yt.YtClient(proxy='hahn', token=self.Parameters.yt_token)
        self.client_yt_path = f'//home/mlcloud/tts/texts_set_operation/{self.Parameters.language}/{self.Parameters.folder_id}'

        start_index = self.Parameters.offset
        end_index = self.Parameters.offset + self.Parameters.texts_number
        if not self.yt_client.exists(f'{self.client_yt_path}/texts_set_snapshot'):
            texts_set = self._get_texts_set(self.Parameters.language)
            self._write_to_yt(texts_set, f'{self.client_yt_path}/texts_set_snapshot', schema)
            texts_set = texts_set[start_index:end_index]
        else:
            table_path = yt.TablePath(f'{self.client_yt_path}/texts_set_snapshot',
                                      start_index=start_index,
                                      end_index=end_index)
            texts_set = [row for row in self.yt_client.read_table(table_path)]

        logger.info(f'got {len(texts_set)} texts')

        result_table_path = ''
        if texts_set:
            result_table_path = f'{self.client_yt_path}/{datetime.datetime.now()}'
            self._write_to_yt([row for row in texts_set], result_table_path, schema)
        json.dump({'cluster': 'hahn', 'table': result_table_path},
                  open(self.Outputs.log_table.get_path(), 'w'))

        with open(self.Outputs.texts.get_path(), 'w') as f:
            for sample in texts_set:
                f.write(f'{sample["uuid"]}\t{sample["text"]}\n')
