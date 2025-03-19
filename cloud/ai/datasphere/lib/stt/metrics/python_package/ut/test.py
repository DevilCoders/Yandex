import pytest
from stt_metrics import evaluate_wer, ClusterReferences


def test_evaluate_wer():
    references = {
        '1': 'ваш звонок был переадресован',
        '2': 'ваш звонок был переадресован',
        '3': 'я же отказалась от него',
        '4': 'я же сказала семь',
        '5': 'привет мир',
        '6': '',
        '7': '',
        '8': 'привет мир'
    }
    hypotheses = {
        '1': 'ваш звонок был переадресован на голоса',
        '2': 'ваш званок был переадресован на голоса',
        '3': 'я же сказала семь',
        '4': 'я же отказалась от него',
        '5': '',
        '6': 'привет мир',
        '7': '',
        '8': 'привет мир'
    }

    expected_wer = {
        '1': 2 / 6,
        '2': 3 / 6,
        '3': 3 / 5,
        '4': 3 / 5,
        '5': 1,
        '6': 1,
        '7': 0,
        '8': 0
    }

    mean_wer, detailed_wer = evaluate_wer(references, hypotheses)

    for key, value in detailed_wer.items():
        assert value['metric_value'] == pytest.approx(expected_wer[key], 1e-6)


def test_evaluate_wer_with_cr():
    references = {
        '1': 'играем в counter strike',
        '2': 'играем в counterstrike',
        '3': 'играем в контер страйк',
        '4': 'играем в контру',
        '5': 'играем в контерстрайк',
        '6': 'играем в контерстрайк',
        '7': 'играем в контерстрайк',
        '8': 'играем в контерстрайк',
    }
    hypotheses = {
        '1': 'играем в контерстрайк',
        '2': 'играем в контерстрайк',
        '3': 'играем в контерстрайк',
        '4': 'играем в контерстрайк',
        '5': 'играем в counter strike',
        '6': 'играем в counterstrike',
        '7': 'играем в контер страйк',
        '8': 'играем в контру',
    }

    cr = ClusterReferences()
    cr.add_cluster('контерстрайк', ['контерстрайк', 'counterstrike', 'контер страйк', 'counter strike'])

    expected_wer = {
        '1': 0,
        '2': 0,
        '3': 0,
        '4': 1 / 3,
        '5': 0,
        '6': 0,
        '7': 0,
        '8': 1 / 3,
    }

    mean_wer, detailed_wer = evaluate_wer(references, hypotheses, cr)

    for key, value in detailed_wer.items():
        assert value['metric_value'] == pytest.approx(expected_wer[key], 1e-6)
