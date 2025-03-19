import os
import unittest

import ujson as json
from voicetech.asr.tools.asr_analyzer.lib.resources import ResourcesPool

from cloud.ai.speechkit.stt.lib.eval.metrics.wer import calculate_errors_ex


class TestErrorsExCalculation(unittest.TestCase):
    def setUp(self):
        # TODO: dirty hack, delete after calculate_errors_ex begins to accept config as json argument
        with open('cluster_references.json', 'w+', encoding='utf-8') as f:
            self.cluster_references_file = f
            self.cluster_references_file.write('{}')

        self.addCleanup(_reset_resource_pool_singleton)

    def tearDown(self):
        os.remove(self.cluster_references_file.name)

    def test_equal_case_insensitive(self):
        result = self._calculate_metrics(
            [
                {"id": "record1", "hyp": "Hello World", "ref": "hello world"},
            ]
        )
        self.assertEqual(0, int(result['WER']))

    def test_insertion(self):
        result = self._calculate_metrics(
            [
                {"id": "record1", "hyp": "hello my pretty world", "ref": "hello world"},
            ]
        )
        self.assertEqual(2, result['InsertedWordsCount'])
        self.assertEqual(100, int(result['WER']))

    def test_deletion(self):
        result = self._calculate_metrics(
            [
                {"id": "record1", "hyp": "hello world", "ref": "hello my world"},
            ]
        )
        self.assertEqual(1, result['DeletedWordsCount'])
        self.assertEqual(33, int(result['WER']))

    def test_substitution(self):
        result = self._calculate_metrics(
            [
                {"id": "record1", "hyp": "hello my dog", "ref": "hello my world"},
            ]
        )
        self.assertEqual(1, result['SubstitutedWordsCount'])
        self.assertEqual(33, int(result['WER']))

    @staticmethod
    def _calculate_metrics(recognitions):
        return json.loads(calculate_errors_ex(recognitions))['results']['by_slice']


def _reset_resource_pool_singleton():
    ResourcesPool.instance = None
