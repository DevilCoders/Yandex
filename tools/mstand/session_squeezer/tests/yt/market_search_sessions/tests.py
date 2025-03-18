import pytest
import test_runner


@pytest.skip("Test must be fixed")
def test_squeeze_pool_market_search_sessions(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["market-search-sessions"],
        pool_path=test_runner.get_pool_path(tests_path=__file__, filename="pool.json"),
        path_to_row_count={
            "//home/mstand/squeeze/testids/market-search-sessions/0/20210715": 34
        }
    )
