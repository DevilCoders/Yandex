import json
import sys
import yatest.common

bench_binary = 'kernel/dssm_applier/optimized_model/bench/bench'


def test_bench(metrics):
    time_budget = yatest.common.get_param('budget', 60)
    result = yatest.common.execute_benchmark(path=yatest.common.binary_path(bench_binary), budget=time_budget)
    sys.stderr.write(json.dumps(result, indent=4))
    metrics.set_benchmark(result)
