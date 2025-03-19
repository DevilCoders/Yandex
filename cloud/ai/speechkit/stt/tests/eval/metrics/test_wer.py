import os
import typing
import unittest

from cloud.ai.speechkit.stt.lib.eval.metrics.calc import get_metric_wer
from cloud.ai.speechkit.stt.lib.eval.metrics.wer import WER, WERData, AlignmentElement, AlignmentAction


class TestWERCalculation(unittest.TestCase):
    WER: WER
    cluster_references_file: typing.IO

    def setUp(self):
        # TODO: dirty hack, delete after calculate_wer begins to accept config as json argument
        with open('cluster_references.json', 'w+', encoding='utf-8') as f:
            self.cluster_references_file = f
            self.cluster_references_file.write(
                """
            {
                "ac dc":         ["acdc", "эйсидиси", "эйси диси"],

                "strikeball":    ["страйк болл"],
                "ball":          ["болл"],
                "strikeballing": ["страйк болл инг"],
                "balling":       ["болл инг"],
                "strike":        ["страйк"]
            }"""
            )
        self.WER = get_metric_wer(self.cluster_references_file.name)

    def tearDown(self):
        os.remove(self.cluster_references_file.name)

    def test_equal_case_insensitive(self):
        self.assertEqual(
            WERData(
                errors_count=0,
                ref_words_count=2,
                hyp_words_count=2,
                diff_hyp='hello world',
                diff_ref='hello world',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='hello',
                        hyp_word='hello',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='world',
                        hyp_word='world',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='Hello World',
                ref='hello world',
            ),
        )

    def test_cluster_reference_center_hyp(self):
        self.assertEqual(
            WERData(
                errors_count=0,
                ref_words_count=4,
                hyp_words_count=4,
                diff_hyp='hello ac dc world',
                diff_ref='hello ac dc world',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='hello',
                        hyp_word='hello',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='ac',
                        hyp_word='ac',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='dc',
                        hyp_word='dc',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='world',
                        hyp_word='world',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='Hello ac dc World',
                ref='hello эйсидиси world',
            ),
        )

    def test_cluster_reference_center_ref(self):
        self.assertEqual(
            WERData(
                errors_count=0,
                ref_words_count=4,
                hyp_words_count=4,
                diff_hyp='hello ac dc world',
                diff_ref='hello ac dc world',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='hello',
                        hyp_word='hello',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='ac',
                        hyp_word='ac',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='dc',
                        hyp_word='dc',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='world',
                        hyp_word='world',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='Hello эйси диси World',
                ref='hello ac dc world',
            ),
        )

    def test_cluster_reference_center_neither(self):
        self.assertEqual(
            WERData(
                errors_count=0,
                ref_words_count=4,
                hyp_words_count=4,
                diff_hyp='hello ac dc world',
                diff_ref='hello ac dc world',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='hello',
                        hyp_word='hello',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='ac',
                        hyp_word='ac',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='dc',
                        hyp_word='dc',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='world',
                        hyp_word='world',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='Hello эйси диси World',
                ref='hello acdc world',
            ),
        )

    def test_cluster_reference_non_trivial(self):
        self.assertEqual(
            WERData(
                errors_count=0,
                ref_words_count=2,
                hyp_words_count=2,
                diff_hyp='игра strikeball',
                diff_ref='игра strikeball',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='игра',
                        hyp_word='игра',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='strikeball',
                        hyp_word='strikeball',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='игра страйк болл',
                ref='игра strikeball',
            )
        )
        self.assertEqual(
            WERData(
                errors_count=0,
                ref_words_count=2,
                hyp_words_count=2,
                diff_hyp='игра strikeballing',
                diff_ref='игра strikeballing',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='игра',
                        hyp_word='игра',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='strikeballing',
                        hyp_word='strikeballing',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='игра страйк болл инг',
                ref='игра strikeballing',
            ),
        )
        self.assertEqual(
            WERData(
                errors_count=0,
                ref_words_count=3,
                hyp_words_count=3,
                diff_hyp='игра strike balling',
                diff_ref='игра strike balling',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='игра',
                        hyp_word='игра',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='strike',
                        hyp_word='strike',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='balling',
                        hyp_word='balling',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='игра страйк болл инг',
                ref='игра strike balling',
            )
        )

    def test_insertion(self):
        self.assertEqual(
            WERData(
                errors_count=2,
                ref_words_count=2,
                hyp_words_count=4,
                diff_hyp='hello my pretty world',
                diff_ref='hello ** ****** world',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='hello',
                        hyp_word='hello',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.INSERTION,
                        ref_word='',
                        hyp_word='my',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.INSERTION,
                        ref_word='',
                        hyp_word='pretty',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='world',
                        hyp_word='world',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='hello my pretty world',
                ref='hello world',
            ),
        )

    def test_deletion(self):
        self.assertEqual(
            WERData(
                errors_count=1,
                ref_words_count=3,
                hyp_words_count=2,
                diff_hyp='hello ** world',
                diff_ref='hello my world',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='hello',
                        hyp_word='hello',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.DELETION,
                        ref_word='my',
                        hyp_word='',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='world',
                        hyp_word='world',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='hello world',
                ref='hello my world',
            ),
        )

    def test_substitution(self):
        self.assertEqual(
            WERData(
                errors_count=1,
                ref_words_count=3,
                hyp_words_count=3,
                diff_hyp='hello UGLY   world',
                diff_ref='hello PRETTY world',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='hello',
                        hyp_word='hello',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.SUBSTITUTION,
                        ref_word='pretty',
                        hyp_word='ugly',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='world',
                        hyp_word='world',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='hello ugly world',
                ref='hello pretty world',
            ),
        )

    def test_multiple_errors(self):
        self.assertEqual(
            WERData(
                errors_count=3,
                ref_words_count=7,
                hyp_words_count=7,
                diff_hyp='**** hello my pretty GREAT ac dc world',
                diff_ref='well hello ** pretty UGLY  ac dc world',
                alignment=[
                    AlignmentElement(
                        action=AlignmentAction.DELETION,
                        ref_word='well',
                        hyp_word='',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='hello',
                        hyp_word='hello',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.INSERTION,
                        ref_word='',
                        hyp_word='my',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='pretty',
                        hyp_word='pretty',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.SUBSTITUTION,
                        ref_word='ugly',
                        hyp_word='great',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='ac',
                        hyp_word='ac',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='dc',
                        hyp_word='dc',
                    ),
                    AlignmentElement(
                        action=AlignmentAction.COINCIDENCE,
                        ref_word='world',
                        hyp_word='world',
                    ),
                ],
            ),
            self.WER.get_metric_data(
                hyp='hello my Pretty great acdc WoRlD',
                ref='well Hello pretty ugly эйси диси world',
            ),
        )

    def test_calculation(self):
        for errors_count, ref_words_count, hyp_words_count, wer in [
            (1, 3, 4, 0.25),
            (1, 4, 3, 0.25),
            (3, 3, 3, 1.0),
            (5, 0, 5, 1.0),
            (5, 5, 0, 1.0),
            (0, 0, 0, 0),
        ]:
            self.assertEqual(wer, self.WER.calculate_metric(WERData(errors_count, ref_words_count, hyp_words_count)))
