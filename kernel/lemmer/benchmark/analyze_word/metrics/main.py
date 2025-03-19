import yatest.common as yc


def test_export_metrics(metrics):
    metrics.set_benchmark(yc.execute_benchmark(
        'kernel/lemmer/benchmark/analyze_word/analyze_word',
        threads=8))
