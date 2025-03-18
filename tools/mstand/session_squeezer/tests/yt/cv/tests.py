import test_runner


def test_squeeze_pool_cv(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["cv"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/cv/399814/20210815": 2
        },
    )
