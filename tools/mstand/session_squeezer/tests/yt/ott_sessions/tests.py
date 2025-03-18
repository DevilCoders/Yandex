import pytest
import test_runner


def test_squeeze_pool_ott_sessions(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["ott-sessions"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/ott-sessions/ott_wiki_boosts.default/20210401": 2
        },
    )


def test_squeeze_empty_pool_ott_sessions_exception(yt_stuff):
    with pytest.raises(Exception, match=r"Empty tables are not allowed.*"):
        test_runner.run_test(
            client=yt_stuff.get_yt_client(),
            services=["ott-sessions"],
            pool_path=test_runner.get_pool_path(__file__, "empty_pool.json"),
            path_to_row_count={
                "//home/mstand/squeeze/testids/ott-sessions/ott_collection_ranking.f28_repeat/20210321": 0
            },
        )


def test_squeeze_empty_pool_ott_sessions_no_exception(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["ott-sessions"],
        pool_path=test_runner.get_pool_path(__file__, "empty_pool.json"),
        path_to_row_count={
            "//home/mstand/squeeze/testids/ott-sessions/ott_collection_ranking.f28_repeat/20210321": 0
        },
        allow_empty_tables=True
    )
