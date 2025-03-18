import pytest
import test_runner


def test_squeeze_pool_market_sessions_stat(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["market-sessions-stat"],
        pool_path=test_runner.get_pool_path(tests_path=__file__, filename="old_pool.json"),
        path_to_row_count={
            "//home/mstand/squeeze/testids/market-sessions-stat/103322/20181027": 1
        },
    )


def test_squeeze_pool_market_sessions_stat_unicode_encode_error_skip(yt_stuff):
    with pytest.raises(Exception, match=r"Empty tables are not allowed.*"):
        test_runner.run_test(
            client=yt_stuff.get_yt_client(),
            services=["market-sessions-stat"],
            pool_path=test_runner.get_pool_path(tests_path=__file__, filename="unicode_decode_error_pool.json"),
            path_to_row_count={
                "//home/mstand/squeeze/testids/market-sessions-stat/0/20200828": 0
            },
        )


def test_squeeze_pool_market_sessions_stat_with_bs_chevent_log(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["market-sessions-stat"],
        pool_path=test_runner.get_pool_path(tests_path=__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/market-sessions-stat/317643/20210205": 2
        },
    )


def test_squeeze_pool_market_web_reqid(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["market-web-reqid"],
        pool_path=test_runner.get_pool_path(tests_path=__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/market-web-reqid/317643/20210205": 2
        }
    )
