import test_runner


def test_squeeze_pool_recommender(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["recommender"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/recommender/100376/20181026": 4
        },
    )
