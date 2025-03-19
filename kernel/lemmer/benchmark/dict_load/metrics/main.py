import yatest.common as yc


def test_export_metrics(metrics):
    metrics.set_benchmark(yc.execute_benchmark(
        'kernel/lemmer/benchmark/dict_load/dict_load',
        threads=8))
