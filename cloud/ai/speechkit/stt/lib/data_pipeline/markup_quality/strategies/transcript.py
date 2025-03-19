import typing
from dataclasses import dataclass

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupSolution,
    SolutionFieldDiff,
    TextComparisonStopWordsArcadiaSource,
    ClusterReferencesInfo,
    SolutionAcceptanceResult,
)
from cloud.ai.speechkit.stt.lib.eval.metrics.wer import WER
from cloud.ai.speechkit.stt.lib.eval.metrics.calc import preprocess_text


@dataclass
class TextComparisonStopWords:
    source: TextComparisonStopWordsArcadiaSource
    words: typing.Set[str]


@dataclass
class TextValidationToolkit:
    wer: WER
    cluster_references_info: ClusterReferencesInfo
    stop_words: TextComparisonStopWords


def transcript_acceptance_strategy_v1(
    expected_solution: MarkupSolution,
    actual_solution: MarkupSolution,
    text_validation_toolkit: TextValidationToolkit,
) -> SolutionAcceptanceResult:
    diff_list = []
    accepted = True

    if expected_solution.type != actual_solution.type:
        diff_list.append(SolutionFieldDiff(
            field_name='type',
            expected_value=expected_solution.type.value,
            actual_value=actual_solution.type.value,
            extra=None,
        ))
        accepted = False

    expected_text, actual_text = expected_solution.text, actual_solution.text
    if expected_text != actual_text:
        diff_list.append(SolutionFieldDiff(
            field_name='text',
            expected_value=expected_text,
            actual_value=actual_text,
            extra={
                'stop-words': text_validation_toolkit.stop_words.source.to_yson(),
                'cr': text_validation_toolkit.cluster_references_info.to_yson(),
            },
        ))

    expected_text_clear, actual_text_clear = (
        preprocess_text(text, text_validation_toolkit.stop_words.words, replace_yo=False)
        for text in (expected_solution.text, actual_solution.text)
    )
    if expected_text_clear != actual_text_clear:
        # run WER alignment just to apply cluster references
        wer_data = text_validation_toolkit.wer.get_metric_data(hyp=actual_text_clear, ref=expected_text_clear)
        if wer_data.errors_count > 0:
            accepted = False

    return SolutionAcceptanceResult(
        accepted=accepted,
        diff_list=diff_list,
    )
