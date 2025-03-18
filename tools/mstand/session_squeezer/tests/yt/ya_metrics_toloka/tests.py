import pytest
import test_runner


@pytest.skip(msg="Service doesn't support py3")
def test_squeeze_pool_ya_metrics_toloka(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["ya-metrics-toloka"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/ya-metrics-toloka/133004/20190925": 1175
        },
    )
