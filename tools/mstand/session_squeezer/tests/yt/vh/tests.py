import test_runner


def test_squeeze_pool_surveys(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["vh"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/vh/173505/20211117": 12,
            "//home/mstand/squeeze/testids/vh/304950/20211117": 12,
            "//home/mstand/squeeze/testids/vh/400700/20211117": 12,
        },
    )
