import yatest.common as yc


def test_matrixnet_apply_metrics(metrics):
    metrics.set_benchmark(yc.execute_benchmark('kernel/matrixnet/perftest/perftest'))
