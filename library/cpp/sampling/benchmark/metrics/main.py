import yatest.common as yc


def test_export_metrics(metrics):
    metrics.set_benchmark(yc.execute_benchmark(
        'library/cpp/sampling/benchmark/benchmark',
        threads=8))
