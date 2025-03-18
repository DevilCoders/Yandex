import pytest
import test_runner


@pytest.skip(msg="Service doesn't support py3")
def test_squeeze_pool_app_metrics(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["app-metrics-toloka"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/app-metrics-toloka/133004/20190801": 578
        },
    )
